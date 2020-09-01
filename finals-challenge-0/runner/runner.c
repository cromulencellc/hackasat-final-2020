#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static uid_t ruid;  // real user id
static uid_t euid;  // effective user id

int main(int argc, char **argv)
{
    int status;

    // make sure we have an argument
    if (argc < 2) {
        return 1;
    }

    // get real and effective user ids
    ruid = getuid();
    euid = geteuid();


    // drop privileges to the effective user
    status = setreuid(ruid, euid);
    if (status < 0) {
        return status;
    }

    // run the supplied command
    execvp(argv[1], argv+1);

    return 0;
}
