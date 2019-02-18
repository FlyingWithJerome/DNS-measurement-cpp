#ifndef NAMETRICK_
#define NAMETRICK_

#ifndef NAMETRICK_EXTERNAL_INCLUDE_

#include <algorithm>
#include <stdlib.h>
#include <vector>

#include <boost/algorithm/string.hpp>

#define NAMES_       1
#define DNS_PACKETS_ 1

#include "../constants.hpp"

#endif

namespace NameTrick
{
    enum JumboType
    {
        no_jumbo,
        jumbo_one_answer,
        jumbo_broken_answer
    };
    
    struct QueryProperty
    {
        QueryProperty(const std::string&);

        ~QueryProperty();

        bool        is_authoritative;
        uint32_t    question_id;
        JumboType   jumbo_type;
        bool        will_truncate;
        std::string name;
    };

    static const long hextable[] = { 
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    uint32_t hex_to_int(const std::string &);
    /* check whether the server is authoritative to the question */
    bool check_authoritative(const std::string&);
    /* get the question ID from the question */
    uint32_t get_question_id(const std::string&);
    /* get the jumbo type from the question */
    JumboType get_jumbo_type(const std::string&);
};

#endif
