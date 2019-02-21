#ifndef NAMETRICK_
#define NAMETRICK_

#ifndef NAMETRICK_EXTERNAL_INCLUDE_

#include <algorithm>
#include <stdlib.h>
#include <string>
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

    /* check whether the server is authoritative to the question */
    bool check_authoritative(const std::string&);
    /* get the question ID from the question */
    uint32_t get_question_id(const std::string&);
    /* get the jumbo type from the question */
    JumboType get_jumbo_type(const std::string&);
};

#endif
