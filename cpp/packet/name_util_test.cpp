#include <gtest/gtest.h>

#include "name_util.hpp"

class NameUtilitiesTest 
: public ::testing::Test
{
    protected:
        void SetUp() override;
    
    std::string name_used_in_normal_udp_discovery;
    std::string name_used_in_sender_over_tcp;
    std::string name_used_in_jumbo_packet_with_acan;
};

void NameUtilitiesTest::SetUp()
{
    name_used_in_normal_udp_discovery   = "0x8080808-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";
    name_used_in_sender_over_tcp        = "t-0x8080404-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";
    name_used_in_jumbo_packet_with_acan = "ac1an10-jumbo1-0x909090a-email-jxm959-case-edu.yumi.ipl.eecs.case.edu";
}

TEST_F(NameUtilitiesTest, ExtractQuestionIDFromNormalUDPDiscovery)
{
    NameUtilities::QueryProperty query_property(name_used_in_normal_udp_discovery);
    ASSERT_EQ(query_property.question_id, 0x8080808);
}

TEST_F(NameUtilitiesTest, ExtractQuestionIDFromSenderOverTCP)
{
    NameUtilities::QueryProperty query_property(name_used_in_sender_over_tcp);
    ASSERT_EQ(query_property.question_id, 0x8080404);
}

TEST_F(NameUtilitiesTest, ExtractQuestionIDFromJumboWithAcan)
{
    NameUtilities::QueryProperty query_property(name_used_in_jumbo_packet_with_acan);
    ASSERT_EQ(query_property.question_id, 0x909090a);
}

TEST_F(NameUtilitiesTest, IdentifySenderOverTCP)
{
    NameUtilities::QueryProperty query_property(name_used_in_sender_over_tcp);
    ASSERT_TRUE(query_property.normal_query_over_tcp);
}

TEST_F(NameUtilitiesTest, IdentifyNotSenderOverTCP)
{
    NameUtilities::QueryProperty query_property(name_used_in_normal_udp_discovery);
    ASSERT_FALSE(query_property.normal_query_over_tcp);
}

TEST_F(NameUtilitiesTest, IdentifyNotSenderOverTCPFromAcan)
{
    NameUtilities::QueryProperty query_property(name_used_in_jumbo_packet_with_acan);
    ASSERT_FALSE(query_property.normal_query_over_tcp);
}

TEST_F(NameUtilitiesTest, ExtractAnswerCountFromJumboWithAcan)
{
    NameUtilities::QueryProperty query_property(name_used_in_jumbo_packet_with_acan);
    ASSERT_EQ(query_property.expect_answer_count, 1);
}

TEST_F(NameUtilitiesTest, ExtractNumberOfAnswersFromJumboWithAcan)
{
    NameUtilities::QueryProperty query_property(name_used_in_jumbo_packet_with_acan);
    ASSERT_EQ(query_property.expect_number_of_answers, 10);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
