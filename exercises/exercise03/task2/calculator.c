/*
Create a program calculator.c which simulates a simple calculator.
The program should accept any basic arithmetical operation as the first argument (+,-,*,/),
followed by at least one number.
The provided operator is then used to calculate the result using floating point arithmetic.
If only one number is provided, this number is the expected result (for any valid operator).
Consider using strtod(3) for parsing the input numbers.

The goal of this exercise is to perform proper error handling and return according exit codes.

Below is a list of constraints for each available exit code:
    return EXIT_SUCCESS if the run is successful
    return 13 if too few arguments are provided (e.g.: ./calculator '+'). In addition, print usage information.
    return 42 if the operator is unknown (e.g.: ./calculator '%' 2.0 3.0 4.0). Again, print usage information.
todo: For better readability, define an enum for non-standard exit codes.
todo: Think of at least two other constraints and add appropriate exit codes.


 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


enum error_codes {
    too_few_arguments = 13,
    operand_not_a_number = 14,
    div_by_zero = 15,
    over_or_underflow = 16,
    operator_unknown = 42,

};


double plus_fkt(double op1, double second_ops[], int number_of_second_ops);
double minus_fkt(double op1, double second_ops[], int number_of_second_ops);
double multiplication_fkt(double op1, double second_ops[], int number_of_second_ops);
double division_fkt(double op1, double second_ops[], int number_of_second_ops);
void div_by_zero_catcher(double second_ops[], int number_of_second_ops);

int main(int argc, char* argv[]) {


    //too_few_arguments check
    if (argc <= 2){
        printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
               "Available operators: +,-,*,/\n");
        return too_few_arguments;
    }

    //check operator_unknon:
    //needs string-comparison
    if( !( (strcmp(argv[1], "+") == 0) || (strcmp(argv[1], "-") == 0) || (strcmp(argv[1], "*") == 0) || (strcmp(argv[1], "/") == 0) ) ){
        printf("%s is not an operator\n", argv[1]);
        printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
               "Available operators: +,-,*,/\n");
        return operator_unknown;
    }
    //operator->char:
    char operator = *argv[1];


    //conversion of first operand:
    errno = 0;
    char* end = NULL;
    double operand_1 = strtod(argv[2], &end);
    //check conversion:
    if( (*end != '\0') || (argv[2] == end) ){       //conversion interrupted || no conversion happened
        printf("first operand not a number!\n");
        printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
               "Available operators: +,-,*,/\n");
        return operand_not_a_number;
    }
    if(errno != 0 ){        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of operand1 ended with error\n");
        printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
               "Available operators: +,-,*,/\n");
        return over_or_underflow;
    }


    //checking for all operands >= 2:

    //printf("Total number of opernds: %d\n", argc);
    //needs to be moved down //printf("number of second opernds: %d\n", number_of_second_ops);
    int number_of_second_ops = argc - 3;
    double second_ops[number_of_second_ops];
    for (int runner = 0; runner < number_of_second_ops; runner++){
        char* end2 = NULL;
        errno = 0;
        int current_op = runner + 3;

        double op = strtod(argv[current_op], &end2);
        //check conversion:
        if( (*end2 != '\0') || (argv[current_op] == end2) ){       //conversion interrupted || no conversion happened
            printf("operand %d not a number!\n", runner + 1);
            printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
                   "Available operators: +,-,*,/\n");
            return operand_not_a_number;
        }
        if(errno != 0 ){        //== ERANGE //as alternative to != 0
            //printf("Overflow or underflow occurred.");
            perror("Conversion of operand1 ended with error");
            printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
                   "Available operators: +,-,*,/\n");
            return over_or_underflow;
        }
        //if valid value -> set the current array variable:
        second_ops[runner] = op;

    }

//    //TODO: DEBUGING - comment out!
//    printf("%f\n", operand_1);
//    for(int i = 0; i < number_of_second_ops; i++){
//        printf("%f\n", second_ops[i]);
//    }
//    //TODO: END




    //calculator implementation:
    double result = 0;

    switch(operator){
        case '+':
            result = plus_fkt(operand_1, second_ops, number_of_second_ops);
            break;
        case '-':
            result = minus_fkt(operand_1, second_ops, number_of_second_ops);
            break;
        case '*':
            result = multiplication_fkt(operand_1, second_ops, number_of_second_ops);
            break;
        case '/':
            div_by_zero_catcher(second_ops, number_of_second_ops);
            result = division_fkt(operand_1, second_ops, number_of_second_ops);
            break;

        default:
            //should never get to this...
            printf("Something went horribly, horribly wrong\n");
            printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
                   "Available operators: +,-,*,/\n");
            return operator_unknown;
    }
/*
TODO:Extend your program to add the value of an environment variable OFFSET to the calculated result.
 For example, when OFFSET is set to 42.5, the call ./calculator '*' 2.0 3.0 should return 48.50.
 todo:Have a look at getenv(3).
 todo:In order to test your program, figure out a way to set/unset environment variables in your shell.
 todo:Additionally, find a way to set the environment variable only for a specific command
    (in this case ./calculator with appropriate arguments).
You can inspect environment variables on your shell with printenv and print them with echo $MY_ENV_VAR.
*/
    //offset implementation
    double offset = 0;

    char* env_var = getenv("OFFSET");
    if(env_var == NULL){
        //no match was found for environment varibale OFFSET
        offset = 0;
    }
    printf("Environmental Variable OFFSET: %s\n\n", env_var);

    //print result
    printf("Result: %f\n", result);






}

double plus_fkt(double op1, double second_ops[], int number_of_second_ops){
    double returnval = op1;
    for(int i = 0; i < number_of_second_ops; i++ ){
        returnval = returnval + second_ops[i];
    }
    return returnval;
}
double minus_fkt(double op1, double second_ops[], int number_of_second_ops){
    double returnval = op1;
    for(int i = 0; i < number_of_second_ops; i++ ){
        returnval = returnval - second_ops[i];
    }
    return returnval;
}
double multiplication_fkt(double op1, double second_ops[], int number_of_second_ops){
    double returnval = op1;
    for(int i = 0; i < number_of_second_ops; i++ ){
        returnval = returnval * second_ops[i];
    }
    return returnval;
}
double division_fkt(double op1, double second_ops[], int number_of_second_ops){
    double returnval = op1;
    for(int i = 0; i < number_of_second_ops; i++ ){
        returnval = returnval / second_ops[i];
    }
    return returnval;
}

void div_by_zero_catcher(double second_ops[], int number_of_second_ops){
    for(int i = 0; i < number_of_second_ops; i++){
        if(second_ops[i] == 0){
            //second operand is zero:
            printf("Imagine that you have zero cookies and you split them evenly among zero friends. See? It doesn’t make sense.\n"
                   "And Cookie Monster is sad that there are no cookies, and you are sad that you have no friends.\n"
                   "--Siri 2015\n\n");
            printf("Usage: ./calculator <operator> <operand1> [operand2 ...]\n"
                   "Available operators: +,-,*,/\n");
            exit(div_by_zero);
        }
    }
}
