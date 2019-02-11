#include "edns.hpp"

EDNS::EDNS(const Tins::DNS& incoming_packet)
{
    if (incoming_packet.additional_count() > 0)
    {
        Tins::DNS::resource EDNS_record = incoming_packet.additional()[incoming_packet.additional_count()-1];

        if (EDNS_record.type() != 41)
        {
            goto No_EDNS_Support;
        }
        else
        {
            EDNS0_payload  = EDNS_record.query_class();

            support_EDNS0  = true;
            support_DNSSEC = (EDNS_record.ttl() & 32768) >> 15; // get the 17th bit (do flag)

            std::string ECS_data = EDNS_record.data();

            bool may_support_ECS = ECS_data.size() != 0;

            if (may_support_ECS)
            {
                uint16_t op_code  = (uint8_t)ECS_data[0] << 8 | (uint8_t)ECS_data[1];
                uint16_t op_len   = (uint8_t)ECS_data[2] << 8 | (uint8_t)ECS_data[3];

                uint16_t family   = (uint8_t)ECS_data[4] << 8 | (uint8_t)ECS_data[5];

                support_ECS = op_code == 8;

                if (support_ECS)
                {
                    ECS_subnet_mask = (int)ECS_data[6];

                    uint32_t subnet_address = 0;

                    for(int index = 7; index < ECS_data.size(); index++)
                        subnet_address = (subnet_address << 8) | (uint8_t)ECS_data[index];

                    subnet_address <<= (32 - ECS_subnet_mask);

                    std::ostringstream address_builder;

                    address_builder 
                    << ((subnet_address >> 24) & 0xFF) 
                    << "."
                    << ((subnet_address >> 16) & 0xFF) 
                    << "."
                    << ((subnet_address >> 8) & 0xFF)
                    << "."
                    << (subnet_address & 0xFF);

                    ECS_subnet_address = address_builder.str();
                    
                    goto Full_Support;
                }
                else
                {
                    goto No_ECS_Support;
                }
            }
            else
            {
                goto No_ECS_Support;
            }
            
        }
    }
    No_EDNS_Support:
    support_EDNS0  = false;
    support_DNSSEC = false;
    EDNS0_payload  = -1;

    No_ECS_Support:
    support_ECS     = false;
    ECS_subnet_mask = 0;
    ECS_subnet_address = "0.0.0.0";
    
    Full_Support: ;
}
