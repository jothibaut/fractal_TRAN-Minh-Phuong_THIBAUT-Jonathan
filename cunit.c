#include <stdlib.h>
#include <stdio.h>
#include "CUnit/CUnit.h";
#include "CUnit/Basic.h";

void test_assert_true(void)
{
  CU_ASSERT(true);
}

void test_assert_2_not_equal_minus_1(void)
{
  CU_ASSERT_NOT_EQUAL(2, -1);
}

void test_string_equals(void)
{
  CU_ASSERT_STRING_EQUAL("string #1", "string #1");
}

void test_failure(void)
{
  CU_ASSERT(false);
}

void test_string_equals_failure(void)
{
  CU_ASSERT_STRING_EQUAL("string #1", "string #2");
}

if (CUE_SUCCESS != CU_initialize_registry())
  return CU_get_error();

int setup(void)  { return 0; }
int teardown(void) { return 0; }

CU_pSuite pSuite = NULL;

pSuite = CU_add_suite("ma_suite", setup, teardown);
if (NULL == pSuite) {
  CU_cleanup_registry();
  return CU_get_error();
}

if ((NULL == CU_add_test(pSuite, "Test assert true", test_assert_true)) ||
    (NULL == CU_add_test(pSuite, "Test assert 2 not equal -1", test_assert_2_not_equal_minus_1)) ||
    (NULL == CU_add_test(pSuite, "Test string equals", test_string_equals)) ||
    (NULL == CU_add_test(pSuite, "Test failure", test_failure)) ||
    (NULL == CU_add_test(pSuite, "Test string equals failure", test_string_equals_failure)))
{
  CU_cleanup_registry();
  return CU_get_error();
}

CU_basic_run_tests();
CU_basic_show_failures(CU_get_failure_list());
