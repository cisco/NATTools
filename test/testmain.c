
#include <stdlib.h>
#include <stdio.h>


#include <check.h>

void
setup (void)
{
        
    
}



void
teardown (void)
{

}



Suite * icelib_suite (void)
{
  Suite *s = suite_create ("ICElib");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  
  //tcase_add_test (tc_core, test_netaddr_create);
  //tcase_add_test (tc_core, test_sockaddr_create);
  //tcase_add_test (tc_core, sockaddr_IPv4_sameaddr);
  //tcase_add_test (tc_core, sockaddr_IPv6_sameaddr);
  //suite_add_tcase (s, tc_core);

  /* Limits test case */
  //TCase *tc_limits = tcase_create ("Limits");
  //tcase_add_test (tc_limits, test_money_create_neg);
  //tcase_add_test (tc_limits, test_money_create_zero);
  //suite_add_tcase (s, tc_limits);

  return s;
}




int main(void){
    
    int number_failed;
    Suite *s = icelib_suite ();
    SRunner *sr = srunner_create (s);
    //srunner_set_fork_status(sr,CK_NOFORK); 
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    

}
