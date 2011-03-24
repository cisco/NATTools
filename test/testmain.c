
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

Suite * icelib_suite (void);
Suite * icelibtypes_suite (void);

int main(void){
    
    int number_failed;
    Suite *s = icelib_suite ();
    SRunner *sr = srunner_create (s);
    srunner_add_suite (sr, icelibtypes_suite ());
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    

}
