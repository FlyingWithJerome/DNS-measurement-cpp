#include "name_util.hpp"

bool NameUtilities::check_authoritative(const std::string& question)
{
    if (AUTHORITATIVE.size() > question.size())
        return false;

    return std::equal(
        question.begin() + question.size() - AUTHORITATIVE.size(), 
        question.end(), 
        AUTHORITATIVE.begin()
    );
}

uint32_t NameUtilities::get_question_id(const std::string& question)
{
    std::vector<std::string> after_split;

    boost::split(after_split, question, boost::is_any_of(DELIMITER));

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
    if (SIGNAL_WORD.compare(question.substr(0, SIGNAL_WORD.size())) != 0)
        return NameUtilities::JumboType(no_jumbo);

    const char key_character = question[SIGNAL_WORD.size()];

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

NameUtilities::QueryProperty::QueryProperty(const std::string& raw_name)
:name(raw_name)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    this->is_authoritative = NameUtilities::check_authoritative(name);
    this->question_id      = NameUtilities::get_question_id(name);
    this->jumbo_type       = NameUtilities::get_jumbo_type(name);

    this->normal_query_over_tcp = (name[0] == 't' and name[1] == '-');

    this->expect_answer_count      = 0;
    this->expect_number_of_answers = 0;

    this->will_truncate    = this->jumbo_type != NameUtilities::JumboType::no_jumbo and TRUNCATION_TRICK;
}

NameUtilities::QueryProperty::~QueryProperty()
{
}

