#include <oisl_42_data_provider.hpp>

namespace Nos3
{
    REGISTER_DATA_PROVIDER(Oisl42DataProvider,"OISL_42_PROVIDER");

    extern ItcLogger::Logger *sim_logger;

    Oisl42DataProvider::Oisl42DataProvider(const boost::property_tree::ptree& config) : SimData42SocketProvider(config)
    {
        sim_logger->trace("Oisl42DataProvider::Oisl42DataProvider:  Constructor executed");

        connect_reader_thread_as_42_socket_client(
            config.get("simulator.hardware-model.data-provider.hostname", "localhost"),
            config.get("simulator.hardware-model.data-provider.port", 4242) );

        _sc = config.get("simulator.hardware-model.data-provider.spacecraft", 0);
    }

    boost::shared_ptr<SimIDataPoint> Oisl42DataProvider::get_data_point(void) const
    {
        sim_logger->trace("Oisl42DataProvider::get_data_point:  Executed");

        /* Get the 42 data */
        const boost::shared_ptr<Sim42DataPoint> dp42 = boost::dynamic_pointer_cast<Sim42DataPoint>(SimData42SocketProvider::get_data_point());

        /* Prepare the specific data */
        SimIDataPoint *dp = new OislDataPoint(_sc, dp42);

        return boost::shared_ptr<SimIDataPoint>(dp);
    }
}
