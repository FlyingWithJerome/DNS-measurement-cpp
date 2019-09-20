#ifndef NAMETRICK_
#define NAMETRICK_

#ifndef NAMETRICK_EXTERNAL_INCLUDE_

#include <algorithm>
#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <map>
#include <vector>

#include <boost/algorithm/string.hpp>

#define NAMES_       1
#define DNS_PACKETS_ 1

#include "../constants.hpp"

#endif

// The incoming queries will have one of the following forms
// 1. 0x8080808-jxm959-case-edu.yumi.ipl.eecs.case.edu
// 2. t-0x8080808-jxm959-case-edu.yumi.ipl.eecs.case.edu
// 3. ac1an1-jumbo1-0x8080808-jxm959-case-edu.yumi.ipl.eecs.case.edu


namespace NameUtilities
{
    const std::string authoritative_name("yumi.ipl.eecs.case.edu");
    const std::string delimiter("-");
    const std::string signal_word("jumbo");
    
    enum JumboType
    {
        no_jumbo,
        jumbo_no_answer,
        jumbo_one_answer,
        jumbo_broken_answer
    };
    
    const std::map<uint8_t, JumboType> int_to_jumbo = {
        {1, jumbo_no_answer},
        {2, jumbo_one_answer},
        {3, jumbo_broken_answer}
    };

    const std::map<JumboType, uint8_t> jumbo_to_int = {
        {jumbo_no_answer,     1},
        {jumbo_one_answer,    2},
        {jumbo_broken_answer, 3}
    };


    
    struct QueryProperty
    {
        QueryProperty() = default;
        QueryProperty(const std::string&);

        bool      is_authoritative;
        bool      will_truncate;
        bool      normal_query_over_tcp;
        uint8_t   expect_answer_count;
        uint8_t   expect_number_of_answers;
        uint32_t  question_id;
        JumboType jumbo_type;

        std::string name;
    };

    /* check whether the server is authoritative to the question */
    bool check_authoritative(const std::string&);
    /* get the question ID from the question */
    uint32_t get_question_id(const std::string&);
    /* get the jumbo type from the question */
    JumboType get_jumbo_type(const std::string&) noexcept;
    /* get the expect answer count and number of answers */
    void get_answer_config(const std::string&, uint8_t& expect_ac, uint8_t& expect_num_ans) noexcept;

    void make_dns_style_string(std::string&);
};

#endif
