#include "name_util.hpp"

bool NameUtilities::check_authoritative(const std::string& question)
{
    if (authoritative_name.size() > question.size())
        return false;

    return std::equal(
        question.begin() + question.size() - authoritative_name.size(), 
        question.end(), 
        authoritative_name.begin()
    );
}

uint32_t NameUtilities::get_question_id(const std::string& question)
{
    std::vector<std::string> after_split;

    boost::split(after_split, question, boost::is_any_of(delimiter));

    if (after_split.size() < 2)
        return INVALID_ID;

    if (after_split[0][0] == '0')
        return (uint32_t)std::strtoul(after_split[0].c_str()+2, nullptr, 16);
    
    else if (after_split[1][0] == '0')
        return (uint32_t)std::strtoul(after_split[1].c_str()+2, nullptr, 16);

    else
        return INVALID_ID;
}

NameUtilities::JumboType NameUtilities::get_jumbo_type(const std::string& question)
{
    // if (signal_word.compare(question.substr(0, signal_word.size())) != 0)
    //     return NameUtilities::JumboType(no_jumbo);

    const char key_character = question[signal_word.size()];

    switch(key_character)
    {
        case '1':
            return NameUtilities::JumboType(jumbo_one_answer);

        case '2':
            return NameUtilities::JumboType(jumbo_broken_answer);
        
        default:
            return NameUtilities::JumboType(no_jumbo);
    }
}

void NameUtilities::get_answer_config(const std::string& name, uint8_t& expect_ac, uint8_t& expect_num_ans)
{
    if ( std::strcmp(name.substr(0, 2).c_str(), "ac") != 0 )
    {
        return;
    }

    std::vector<std::string> after_split;
    boost::split(after_split, name, boost::is_any_of(std::string("a")));

    if (after_split.size() != 3) /* the first one should be empty */
    {
        return;
    }

    if ( 
        after_split[1][0] != 'c' /* c<int> */
        or
        after_split[2][0] != 'n' /* n<int> */
    )
    {
        return;
    }

    expect_ac      = (uint8_t)std::strtoul(after_split[1].c_str()+1, nullptr, 10);
    expect_num_ans = (uint8_t)std::strtoul(after_split[2].c_str()+1, nullptr, 10);
}

NameUtilities::QueryProperty::QueryProperty(const std::string& raw_name)
: name(raw_name)
{
    jumbo_type = JumboType::no_jumbo;

    expect_answer_count      = UINT8_MAX;
    expect_number_of_answers = UINT8_MAX;

    question_id = 0;
    normal_query_over_tcp = false;

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    std::vector<std::string> after_split;
    boost::split(after_split, name, boost::is_any_of(delimiter));

    for (const std::string& section : after_split)
    {
        switch (section[0])
        {
            case 't':
                normal_query_over_tcp = true;
                break;
            
            case 'j':
                if (section[1] == 'u')
                    jumbo_type = NameUtilities::get_jumbo_type(section);
                break;
            
            case 'a':
                get_answer_config(section, expect_answer_count, expect_number_of_answers);
                break;

            case '0':
                question_id = (uint32_t)std::strtoul(section.c_str()+2, nullptr, 16);
                break;

            default:
                break;
        }
    }

    is_authoritative = NameUtilities::check_authoritative(name);
    will_truncate    = (jumbo_type != NameUtilities::JumboType::no_jumbo and TRUNCATION_TRICK);
}

