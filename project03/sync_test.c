#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define FSIZE0 (1024)
#define FSIZE1 (1024 * 1024 * 8)
#define FSIZE2 (1024 * 1024 * 16)

char buffer0[FSIZE0], buffer1[FSIZE1], buffer2[FSIZE2];

void
test0()
{
    int fd = 0;

     // create a file
    if((fd = open("stest0", O_CREATE | O_RDWR)) < 0){
        printf(1, "sync_test: create failed\n");
        exit();
    }
    printf(1, "sync_test: create success\n");

    // write buffer to the file
    if(write(fd, buffer0, FSIZE0) < 0){
        printf(1, "sync_test: write failed\n");
        exit();
    }
    printf(1, "sync_test: write success\n");

    // read buffer from the file
    if(read(fd, buffer0, FSIZE0) < 0){
        printf(1, "sync_test: read failed\n");
        exit();
    }

    for(int i = 0; i < FSIZE0; i++){
        if(buffer0[i] != '0' + (i%10)){
            printf(1, "sync_test: read failed\n");
            exit();
        }
    }
    printf(1, "sync_test: read success\n");

    // close the file
    if(close(fd) < 0){
        printf(1, "sync_test: close failed\n");
        exit();
    }

    // remove the file
    if(unlink("stest0") < 0){
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
    printf(1, "sync_test: close success\n");
    
    // check if file is removed
    if((fd = open("stest0", O_RDONLY)) < 0){
        printf(1, "sync_test: unlink success\n");
    }
    else{
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
}

void
test1()
{
    int fd = 0;

     // create a file
    if((fd = open("stest1", O_CREATE | O_RDWR)) < 0){
        printf(1, "sync_test: create failed\n");
        exit();
    }
    printf(1, "sync_test: create success\n");

    // write buffer to the file
    if(write(fd, buffer1, FSIZE1) < 0){
        printf(1, "sync_test: write failed\n");
        exit();
    }
    printf(1, "sync_test: write success\n");

    // read buffer from the file
    if(read(fd, buffer1, FSIZE1) < 0){
        printf(1, "sync_test: read failed\n");
        exit();
    }

    for(int i = 0; i < FSIZE1; i++){
        if(buffer1[i] != '0' + (i%10)){
            printf(1, "sync_test: read failed\n");
            exit();
        }
    }
    printf(1, "sync_test: read success\n");

    // close the file
    if(close(fd) < 0){
        printf(1, "sync_test: close failed\n");
        exit();
    }

    // remove the file
    if(unlink("stest1") < 0){
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
    printf(1, "sync_test: close success\n");
    
    // check if file is removed
    if((fd = open("stest1", O_RDONLY)) < 0){
        printf(1, "sync_test: unlink success\n");
    }
    else{
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
}

void
test2()
{
    int fd = 0;

    sync();

     // create a file
    if((fd = open("stest2", O_CREATE | O_RDWR)) < 0){
        printf(1, "sync_test: create failed\n");
        exit();
    }
    printf(1, "sync_test: create success\n");

    // write buffer to the file
    if(write(fd, buffer2, FSIZE2) < 0){
        printf(1, "sync_test: write failed\n");
        exit();
    }
    int i = sync();
    printf(1, "sync_test: write success(%d)\n", i);

    // read buffer from the file
    if(read(fd, buffer2, FSIZE2) < 0){
        printf(1, "sync_test: read failed\n");
        exit();
    }

    for(int i = 0; i < FSIZE2; i++){
        if(buffer2[i] != '0' + (i%10)){
            printf(1, "sync_test: read failed\n");
            exit();
        }
    }
    printf(1, "sync_test: read success\n");

    // close the file
    if(close(fd) < 0){
        printf(1, "sync_test: close failed\n");
        exit();
    }
    printf(1, "sync_test: close success\n");

    // remove the file
    if(unlink("stest2") < 0){
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
    // check if file is removed
    if((fd = open("stest2", O_RDONLY)) < 0){
        printf(1, "sync_test: unlink success\n");
    }
    else{
        printf(1, "sync_test: unlink failed\n");
        exit();
    }
}

int main(int argc, char *argv[])
{
    printf(1, "sync_test: start\n");

    // Set buffer0
    for(int i = 0; i < FSIZE0; i++){
        buffer0[i] = '0' + (i%10);
    }

    // Set buffer1
    for(int i = 0; i < FSIZE1; i++) {
        buffer1[i] = '0' + (i%10);
    }

    // Set buffer2
    for(int i = 0; i < FSIZE2; i++) {
        buffer2[i] = '0' + (i%10);
    }

    printf(1, "Test1 0: write 1KB (%d) file \n", FSIZE0);
    test0();
    printf(1, "Test 0 done\n");

    printf(1, "Test 1: write 8MB (%d) file\n", FSIZE1);
    test1();
    printf(1, "Test 1 done\n");

    printf(1, "Test 2: write 16MB (%d) file\n", FSIZE2);
    test2();
    printf(1, "Test 2 done\n");

    printf(1, "sync_test: end\n");

    exit();
}