#include <oisl_hardware_model.hpp>

namespace Nos3
{
    REGISTER_HARDWARE_MODEL(OislHardwareModel,"OISL");

    extern ItcLogger::Logger *sim_logger;

    OislHardwareModel::OislHardwareModel(const boost::property_tree::ptree& config) : SimIHardwareModel(config), 
    _enabled(OISL_SIM_SUCCESS), _count(0), _config(0), _status(0)
    {
        /* Get the NOS engine connection string */
        std::string connection_string = config.get("common.nos-connection-string", "tcp://127.0.0.1:12001"); 
        sim_logger->info("OislHardwareModel::OislHardwareModel:  NOS Engine connection string: %s.", connection_string.c_str());

        /* Get a data provider */
        std::string dp_name = config.get("simulator.hardware-model.data-provider.type", "OISL_PROVIDER");
        _oisl_dp = SimDataProviderFactory::Instance().Create(dp_name, config);
        sim_logger->info("OislHardwareModel::OislHardwareModel:  Data provider %s created.", dp_name.c_str());

        /* Get on a protocol bus */
        /* Note: Initialized defaults in case value not found in config file */
        std::string bus_name = "usart_29";
        int node_port = 29;
        if (config.get_child_optional("simulator.hardware-model.connections")) 
        {
            /* Loop through the connections for hardware model */
            BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, config.get_child("simulator.hardware-model.connections"))
            {
                /* v.second is the child tree (v.first is the name of the child) */
                if (v.second.get("type", "").compare("usart") == 0)
                {
                    /* Configuration found */
                    bus_name = v.second.get("bus-name", bus_name);
                    node_port = v.second.get("node-port", node_port);
                    break;
                }
            }
        }
        _uart_connection.reset(new NosEngine::Uart::Uart(_hub, config.get("simulator.name", "oisl_sim"), connection_string, bus_name));
        _uart_connection->open(node_port);
        sim_logger->info("OislHardwareModel::OislHardwareModel:  Now on UART bus name %s, port %d.", bus_name.c_str(), node_port);
    
        /* Configure protocol callback */
        _uart_connection->set_read_callback(std::bind(&OislHardwareModel::uart_read_callback, this, std::placeholders::_1, std::placeholders::_2));

        /* Get on the command bus*/
        std::string time_bus_name = "command";
        if (config.get_child_optional("hardware-model.connections")) 
        {
            /* Loop through the connections for the hardware model */
            BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, config.get_child("hardware-model.connections"))
            {
                /* v.first is the name of the child */
                /* v.second is the child tree */
                if (v.second.get("type", "").compare("time") == 0) // 
                {
                    time_bus_name = v.second.get("bus-name", "command");
                    /* Found it... don't need to go through any more items*/
                    break; 
                }
            }
        }
        _time_bus.reset(new NosEngine::Client::Bus(_hub, connection_string, time_bus_name));
        sim_logger->info("OislHardwareModel::OislHardwareModel:  Now on time bus named %s.", time_bus_name.c_str());

        /* Remove the VoV file at startup if it exists */
        if (remove(VoV_forward_filename) == 0) {
            sim_logger->debug("File VoV deleted successfully.\n");
        } else {
            sim_logger->debug("Failed to delete the VoV file.\n");
        }
        /* Remove the VoV file at startup if it exists */
        if (remove(VoV_backward_filename) == 0) {
            sim_logger->debug("File VoV deleted successfully.\n");
        } else {
            sim_logger->debug("Failed to delete the VoV file.\n");
        }
        /* Remove the VoV file at startup if it exists */
        if (remove(VoV_OGS_filename) == 0) {
            sim_logger->debug("File VoV deleted successfully.\n");
        } else {
            sim_logger->debug("Failed to delete the VoV file.\n");
        }
        
        /* Construction complete */
        sim_logger->info("OislHardwareModel::OislHardwareModel:  Construction complete.");
    }


    OislHardwareModel::~OislHardwareModel(void)
    {        
        /* Close the protocol bus */
        _uart_connection->close();

        /* Clean up the data provider */
        delete _oisl_dp;
        _oisl_dp = nullptr;

        /* The bus will clean up the time node */
    }


    /* Automagically set up by the base class to be called */
    void OislHardwareModel::command_callback(NosEngine::Common::Message msg)
    {
        /* Get the data out of the message */
        NosEngine::Common::DataBufferOverlay dbf(const_cast<NosEngine::Utility::Buffer&>(msg.buffer));
        sim_logger->info("OislHardwareModel::command_callback:  Received command: %s.", dbf.data);

        /* Do something with the data */
        std::string command = dbf.data;
        std::string response = "OislHardwareModel::command_callback:  INVALID COMMAND! (Try HELP)";
        boost::to_upper(command);
        if (command.compare("HELP") == 0) 
        {
            response = "OislHardwareModel::command_callback: Valid commands are HELP, ENABLE, DISABLE, STATUS=X, or STOP";
        }
        else if (command.compare(0,6,"ENABLE") == 0) 
        {
            _enabled = OISL_SIM_SUCCESS;
            response = "OislHardwareModel::command_callback:  Enabled\n";
        }
        else if (command.compare(0,7,"DISABLE") == 0) 
        {
            _enabled = OISL_SIM_ERROR;
            _count = 0;
            _config = 0;
            _status = 0;
            response = "OislHardwareModel::command_callback:  Disabled";
        }
        else if (command.substr(0,7).compare("STATUS=") == 0)
        {
            try
            {
                _status = std::stod(command.substr(7));
                response = "OislHardwareModel::command_callback:  Status set";
            }
            catch (...)
            {
                response = "OislHardwareModel::command_callback:  Status invalid";
            }            
        }
        else if (command.compare(0,4,"STOP") == 0) 
        {
            _keep_running = false;
            response = "OislHardwareModel::command_callback:  Stopping";
        }
        /* TODO: Add anything additional commands here */

        /* Send a reply */
        sim_logger->info("OislHardwareModel::command_callback:  Sending reply: %s", response.c_str());
        _command_node->send_reply_message_async(msg, response.size(), response.c_str());
    }


    /* Custom function to prepare the Oisl HK telemetry */
    void OislHardwareModel::create_oisl_hk(std::vector<uint8_t>& out_data)
    {
        /* Prepare data size */
        out_data.resize(16, 0x00);

        /* Streaming data header - 0xDEAD */
        out_data[0] = 0xDE;
        out_data[1] = 0xAD;
        
        /* Sequence count */
        out_data[2] = (_count >> 24) & 0x000000FF; 
        out_data[3] = (_count >> 16) & 0x000000FF; 
        out_data[4] = (_count >>  8) & 0x000000FF; 
        out_data[5] =  _count & 0x000000FF;
        
        /* Configuration */
        out_data[6] = (_config >> 24) & 0x000000FF; 
        out_data[7] = (_config >> 16) & 0x000000FF; 
        out_data[8] = (_config >>  8) & 0x000000FF; 
        out_data[9] =  _config & 0x000000FF;

        /* Device Status */
        out_data[10] = (_status >> 24) & 0x000000FF; 
        out_data[11] = (_status >> 16) & 0x000000FF; 
        out_data[12] = (_status >>  8) & 0x000000FF; 
        out_data[13] =  _status & 0x000000FF;

        /* Streaming data trailer - 0xBEEF */
        out_data[14] = 0xBE;
        out_data[15] = 0xEF;
    }


    /* Custom function to prepare the Oisl Data */
    void OislHardwareModel::create_oisl_data(std::vector<uint8_t>& out_data)
    {

        /* Prepare data size */
        out_data.resize(11, 0x00);

        /* Streaming data header - 0xDEAD */
        out_data[0] = 0xDE;
        out_data[1] = 0xAD;
        
        /* Sequence count */
        out_data[2] = (_count >> 24) & 0x000000FF; 
        out_data[3] = (_count >> 16) & 0x000000FF; 
        out_data[4] = (_count >>  8) & 0x000000FF; 
        out_data[5] =  _count & 0x000000FF;
        
        /* 
        ** Payload 
        ** 
        ** Device is big engian (most significant byte first)
        ** Assuming data is valid regardless of dynamic / environmental data
        ** Floating poing numbers are extremely problematic 
        **   (https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html)
        ** Most hardware transmits some type of unsigned integer (e.g. from an ADC), so that's what we've done
        ** Scale each of the x, y, z (which are in the range [-1.0, 1.0]) by 32767, 
        **   and add 32768 so that the result fits in a uint16
        */

        /* Retrieving VoV for Forward and Backward alignment */
        FILE *file_VoV_F = fopen(VoV_forward_filename, "r");
        double VoV_F = 0.0;  // Default value
        if (file_VoV_F != NULL) {
            fscanf(file_VoV_F, "%lf", &VoV_F);
            fclose(file_VoV_F);
        } else {
            sim_logger->debug("File FORWARD cannot be opened or does not exist. Setting x to default value: %f\n", VoV_F);
        }
        uint8_t forward_alignment = (VoV_F >= cos(FoR)) ? 1 : 0; 
        // DELETE FILE ONCE THE VALUE IS READ. If the ADCS mode is active, the file will be created again.
        remove(VoV_forward_filename);

        FILE *file_VoV_B = fopen(VoV_backward_filename, "r");
        double VoV_B = 0.0;  // Default value
        if (file_VoV_B != NULL) {
            fscanf(file_VoV_B, "%lf", &VoV_B);
            fclose(file_VoV_B);
        } else {
            sim_logger->debug("File backward cannot be opened or does not exist. Setting x to default value: %f\n", VoV_B);
        }
        uint8_t backward_alignment = (VoV_B >= cos(FoR)) ? 1 : 0; 
        // DELETE FILE ONCE THE VALUE IS READ. If the ADCS mode is active, the file will be created again.
        remove(VoV_backward_filename);

        out_data[6] = forward_alignment & 0x00FF;
        out_data[7] = backward_alignment & 0x00FF;

        sim_logger->debug("OislHardwareModel::create_oisl_data  ALIGNED: =  %u, %u. VoV values for foward and backward: %f %f %f\n", forward_alignment,  backward_alignment, VoV_F, VoV_B, cos(FoR));

        // OGS Alignment
        FILE *file_VoV_OGS = fopen(VoV_OGS_filename, "r");
        double VoV_OGS = 0.0;  // Default value
        if (file_VoV_OGS != NULL) {
            fscanf(file_VoV_OGS, "%lf", &VoV_OGS);
            fclose(file_VoV_OGS);
        } else {
            sim_logger->debug("File OGS cannot be opened or does not exist. Setting x to default value: %f\n", VoV_OGS);
        }
        uint8_t OGS_alignment = (VoV_OGS >= cos(FoR_OGS)) ? 1 : 0;
        // DELETE FILE ONCE THE VALUE IS READ. If the ADCS mode is active, the file will be created again.
        remove(VoV_OGS_filename);

        out_data[8] = OGS_alignment & 0x00FF;

        /* Streaming data trailer - 0xBEEF */
        out_data[9] = 0xBE;
        out_data[10] = 0xEF;
    }


    /* Protocol callback */
    void OislHardwareModel::uart_read_callback(const uint8_t *buf, size_t len)
    {
        std::vector<uint8_t> out_data; 
        std::uint8_t valid = OISL_SIM_SUCCESS;
        
        /* Retrieve data and log in man readable format */
        std::vector<uint8_t> in_data(buf, buf + len);
        sim_logger->debug("OislHardwareModel::uart_read_callback:  REQUEST %s",
            SimIHardwareModel::uint8_vector_to_hex_string(in_data).c_str());

        /* Check simulator is enabled */
        if (_enabled != OISL_SIM_SUCCESS)
        {
            sim_logger->debug("OislHardwareModel::uart_read_callback:  Oisl sim disabled!");
            valid = OISL_SIM_ERROR;
        }
        else
        {
            /* Check if message is incorrect size */
            if (in_data.size() != 9)
            {
                sim_logger->debug("OislHardwareModel::uart_read_callback:  Invalid command size of %ld received!", in_data.size());
                valid = OISL_SIM_ERROR;
            }
            else
            {
                /* Check header - 0xDEAD */
                if ((in_data[0] != 0xDE) || (in_data[1] !=0xAD))
                {
                    sim_logger->debug("OislHardwareModel::uart_read_callback:  Header incorrect!");
                    valid = OISL_SIM_ERROR;
                }
                else
                {
                    /* Check trailer - 0xBEEF */
                    if ((in_data[7] != 0xBE) || (in_data[8] !=0xEF))
                    {
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  Trailer incorrect!");
                        valid = OISL_SIM_ERROR;
                    }
                    else
                    {
                        /* Increment count as valid command format received */
                        _count++;
                    }
                }
            }

            if (valid == OISL_SIM_SUCCESS)
            {   
                /* Process command */
                switch (in_data[2])
                {
                    case 0:
                        /* NOOP */
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  NOOP command received!");
                        break;

                case 1:
                        /* Request HK */
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  Send HK command received!");
                        create_oisl_hk(out_data);
                        break;

                    case 2:
                        /* Request data */
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  Send data command received!");
                        create_oisl_data(out_data);
                        break;

                    case 3:
                        /* Configuration */
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  Configuration command received!");
                        _config  = in_data[3] << 24;
                        _config |= in_data[4] << 16;
                        _config |= in_data[5] << 8;
                        _config |= in_data[6];
                        break;
                    
                    default:
                        /* Unused command code */
                        valid = OISL_SIM_ERROR;
                        sim_logger->debug("OislHardwareModel::uart_read_callback:  Unused command %d received!", in_data[2]);
                        break;
                }
            }
        }

        /* Echo command since format valid */
        if (valid == OISL_SIM_SUCCESS)
        {
            _uart_connection->write(&in_data[0], in_data.size());

            /* Send response if existing */
            if (out_data.size() > 0)
            {
                sim_logger->debug("OislHardwareModel::uart_read_callback:  REPLY %s",
                    SimIHardwareModel::uint8_vector_to_hex_string(out_data).c_str());
                _uart_connection->write(&out_data[0], out_data.size());
            }
        }
    }
}
