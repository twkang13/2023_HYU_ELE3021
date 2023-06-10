#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define FSIZE1 (1024 * 1024 * 8)
#define FSIZE2 (1024 * 1024 * 16)

char buffer1[FSIZE1], buffer2[FSIZE2];

void
test1()
{
    int fd = 0;

     // create a file
    if((fd = open("mtest1", O_CREATE | O_RDWR)) < 0){
        printf(1, "multi_test: create failed\n");
        exit();
    }
    printf(1, "multi_test: create success\n");

    // write buffer to the file
    if(write(fd, buffer1, FSIZE1) < 0){
        printf(1, "multi_test: write failed\n");
        exit();
    }
    printf(1, "multi_test: write success\n");

    // read buffer from the file
    if(read(fd, buffer1, FSIZE1) < 0){
        printf(1, "multi_test: read failed\n");
        exit();
    }

    for(int i = 0; i < FSIZE1; i++){
        if(buffer1[i] != '0' + (i%10)){
            printf(1, "multi_test: read failed\n");
            exit();
        }
    }
    printf(1, "multi_test: read success\n");

    // close the file
    if(close(fd) < 0){
        printf(1, "multi_test: close failed\n");
        exit();
    }

    // remove the file
    if(unlink("mtest1") < 0){
        printf(1, "multi_test: unlink failed\n");
        exit();
    }
    printf(1, "multi_test: close success\n");
    
    // check if file is removed
    if((fd = open("mtest1", O_RDONLY)) < 0){
        printf(1, "multi_test: unlink success\n");
    }
    else{
        printf(1, "multi_test: unlink failed\n");
        exit();
    }
}

void
test2()
{
    int fd = 0;

     // create a file
    if((fd = open("mtest1", O_CREATE | O_RDWR)) < 0){
        printf(1, "multi_test: create failed\n");
        exit();
    }
    printf(1, "multi_test: create success\n");

    // write buffer to the file
    if(write(fd, buffer2, FSIZE2) < 0){
        printf(1, "multi_test: write failed\n");
        exit();
    }
    printf(1, "multi_test: write success\n");

    // read buffer from the file
    if(read(fd, buffer2, FSIZE2) < 0){
        printf(1, "multi_test: read failed\n");
        exit();
    }

    for(int i = 0; i < FSIZE2; i++){
        if(buffer2[i] != '0' + (i%10)){
            printf(1, "multi_test: read failed\n");
            exit();
        }
    }
    printf(1, "multi_test: read success\n");

    // close the file
    if(close(fd) < 0){
        printf(1, "multi_test: close failed\n");
        exit();
    }
    printf(1, "multi_test: close success\n");

    // remove the file
    if(unlink("mtest1") < 0){
        printf(1, "multi_test: unlink failed\n");
        exit();
    }
    // check if file is removed
    if((fd = open("mtest1", O_RDONLY)) < 0){
        printf(1, "multi_test: unlink success\n");
    }
    else{
        printf(1, "multi_test: unlink failed\n");
        exit();
    }
}

int main(int argc, char *argv[])
{
    printf(1, "multi_test: start\n");

    // Set buffer1
    for(int i = 0; i < FSIZE1; i++) {
        buffer1[i] = '0' + (i%10);
    }

    // Set buffer2
    for(int i = 0; i < FSIZE2; i++) {
        buffer2[i] = '0' + (i%10);
    }

    printf(1, "Test 1: write 8MB (%d) file\n", FSIZE1);
    test1();
    printf(1, "Test 1 done\n");

    printf(1, "Test 2: write 16MB (%d) file\n", FSIZE2);
    test2();
    printf(1, "Test 2 done\n");

    printf(1, "multi_test: end\n");

    exit();
}