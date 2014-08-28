
#include <stdlib.h>
#include <stdio.h>
#include <check.h>


Suite * stunlib_suite (void);
Suite * turnclient_suite (void);
Suite * turnmessage_suite (void);
Suite * realworldmsg_suite (void);
Suite * stunclient_suite (void);
Suite * stunserver_suite (void);


int main(void){

    int number_failed;
    //SRunner *sr = srunner_create (turnmessage_suite ());
    SRunner *sr = srunner_create (stunlib_suite ());
    //SRunner *sr = srunner_create (s);
    srunner_add_suite (sr, turnclient_suite ());
    srunner_add_suite (sr, turnmessage_suite ());
    srunner_add_suite (sr, realworldmsg_suite ());
    srunner_add_suite (sr, stunclient_suite ());
    srunner_add_suite (sr, stunserver_suite ());
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
