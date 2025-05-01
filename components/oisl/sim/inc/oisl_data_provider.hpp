#ifndef NOS3_OISLDATAPROVIDER_HPP
#define NOS3_OISLDATAPROVIDER_HPP

#include <boost/property_tree/xml_parser.hpp>
#include <ItcLogger/Logger.hpp>
#include <oisl_data_point.hpp>
#include <sim_i_data_provider.hpp>

namespace Nos3
{
    class OislDataProvider : public SimIDataProvider
    {
    public:
        /* Constructors */
        OislDataProvider(const boost::property_tree::ptree& config);

        /* Accessors */
        boost::shared_ptr<SimIDataPoint> get_data_point(void) const;

    private:
        /* Disallow these */
        ~OislDataProvider(void) {};
        OislDataProvider& operator=(const OislDataProvider&) {return *this;};

        mutable double _request_count;
    };
}

#endif
