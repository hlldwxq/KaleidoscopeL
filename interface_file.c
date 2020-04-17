#include <stdio.h>

double func();
double pi();
double test();
double test2();
double fib5();


int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {

//   double res = 0;
//   double sign = 1;
//   for (int i=1;i<100000;i+=2) {
//     res = res + sign/i;
//     sign=-sign;
//   }
//  res*=4;

   double res = fib5(2.0);
   printf("Result %lf\n",res);
}





