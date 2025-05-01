#include <oisl_data_provider.hpp>

namespace Nos3
{
    REGISTER_DATA_PROVIDER(OislDataProvider,"OISL_PROVIDER");

    extern ItcLogger::Logger *sim_logger;

    OislDataProvider::OislDataProvider(const boost::property_tree::ptree& config) : SimIDataProvider(config)
    {
        sim_logger->trace("OislDataProvider::OislDataProvider:  Constructor executed");
        _request_count = 0;
    }

    boost::shared_ptr<SimIDataPoint> OislDataProvider::get_data_point(void) const
    {
        sim_logger->trace("OislDataProvider::get_data_point:  Executed");

        /* Prepare the provider data */
        _request_count++;

        /* Request a data point */
        SimIDataPoint *dp = new OislDataPoint(_request_count);

        /* Return the data point */
        return boost::shared_ptr<SimIDataPoint>(dp);
    }
}
