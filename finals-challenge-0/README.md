# Hack-a-Sat Finals Challenge 0 #

This is the initial challenge for the 2020 Hack-a-Sat final event. It is
intended to be a small, initial step on the way to the ground station in the
form of an old, vulnerable web app. This challenge should be the easiest one
since we're not actually on the satellite yet. Think of it like a sanity check
challenge in a more traditional CTF.


## Installation ##

Copy the files from this repository over:

```sh
# replace X with the team we're deploying to (1-8)
rsync -avzd finals-challenge-0 root@10.100.X.10:
```

Then, SSH in and run the install script:

```sh
# replace X with the team we're deploying to (1-8)
ssh root@10.100.X.10
TEAM=X finals-challenge-0/install.sh
```


## Deployment ##

Checklist:

* Going to http://10.100.X.10/ must show you the web application
* Going to http://10.100.X.10/notes must not show a database error
* Going to http://10.100.X.10/admin must show an error page that includes the secret
* Logging in as "admin" on http://10.100.X.10/ with no password must not work
* `/usr/local/bin/runner` must exist with permissions `rwsr-xr-x.  1 user user` and MD5 bf95aeeb1791105ad92315f9bd80bdac
* `/home/user/.ssh/authorized_keys` must exist with content matching `backdoor/id_rsa.pub` from this repo
* `/usr/local/bin/runner` must be capable of writing to `/home/user/.ssh/authorized_keys`
* `/home/op1/.ssh/authorized_keys` on 10.100.X.5 must exist with content matching `/home/user/.ssh/id_rsa.pub` on 10.100.X.10
* `/root/finals-challenge-0` must no longer exist
