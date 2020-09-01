#!/bin/bash

# this file is meant to be executed on the host after having this repository
# rsync'd to it as /root/finals-challenge-0

# we do some things that are unique to each team, so you must set the TEAM environment variable before running
if [[ -z "${TEAM}" ]]; then
    echo "You need to set the TEAM environment variable to a number 1-8!"
    exit 1
fi

# install useful developer tools
yum -y update && yum -y install vim tmux rsync git gcc gdb make which

# install database server
yum -y install postgresql postgresql-devel postgresql-server postgresql-contrib
postgresql-setup initdb
sed -i "/listen_addresses =/s/^#*//g" /var/lib/pgsql/data/postgresql.conf \
  && sed -i "/listen_addresses =/s/localhost/\*/g" /var/lib/pgsql/data/postgresql.conf \
  && sed -i "/port =/s/^#*//g" /var/lib/pgsql/data/postgresql.conf
sed -i "/ident$/s/ident/md5/g" /var/lib/pgsql/data/pg_hba.conf
echo "host    all             all             10.100.$TEAM.5/32           md5" >> /var/lib/pgsql/data/pg_hba.conf
systemctl start postgresql

# create database with user (ignore the "could not change directory" messages)
sudo -u postgres createdb launchsip
sudo -u postgres psql -c "CREATE USER launchsip WITH PASSWORD 'wroashsoxDiculReejLykUssyifabEdGhovHabno';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE launchsip to launchsip;"

# install version of ruby that isn't 8+ years old
yum -y install gcc g++ patch readline readline-devel zlib zlib-devel openssl-devel \
  make bzip2 autoconf automake libtool bison iconv-devel libyaml-devel libffi-devel
if [ ! -f /etc/profile.d/rvm.sh ]; then
gpg --keyserver hkp://na.pool.sks-keyservers.net --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3 7D2BAF1CF37B13E2069D6956105BD0E739499BDB \
  && curl -L get.rvm.io | bash -s stable \
  && echo "source /etc/profile.d/rvm.sh" >> ~/.bash_profile \
  && /bin/bash -l -c "rvm reload && rvm install 2.7"
fi
source /etc/profile.d/rvm.sh

# turn off SELinux because we're moving fast and I haven't figured out how to un-break it yet
setenforce 0

# disable the firewall because we're moving fast and I haven't hardened this yet
systemctl stop firewalld

# install apache web server and mod_passenger
yum -y install httpd httpd-devel libcurl-devel apr-devel apr-util-devel
if [ ! -f /etc/httpd/conf.d/passenger.conf ]; then
gem install passenger && passenger-install-apache2-module  # this command requires user interaction (just use defaults)
cat <<EOF > /etc/httpd/conf.d/passenger.conf
LoadModule passenger_module /usr/local/rvm/gems/ruby-2.7.0/gems/passenger-6.0.6/buildout/apache2/mod_passenger.so
<IfModule mod_passenger.c>
  PassengerRoot /usr/local/rvm/gems/ruby-2.7.0/gems/passenger-6.0.6
  PassengerDefaultRuby /usr/local/rvm/gems/ruby-2.7.0/wrappers/ruby
</IfModule>
EOF
fi
systemctl restart httpd
passenger-config validate-install
cat <<EOF > /etc/httpd/conf.d/launchsip.conf
<VirtualHost *:80>
    ServerName launchsip
    ServerAlias launchsip.lan
    DocumentRoot /var/www/launchsip/content

    <Directory /var/www/launchsip/content>
            Order allow,deny
            Allow from all
            AllowOverride All
            Options -MultiViews
    </Directory>
</VirtualHost>
EOF
cp -r finals-challenge-0/launchsip /var/www/
cd /var/www/launchsip
touch logs/sinatra.log
chmod 777 logs/sinatra.log
rm -rf .bundle bundle
bundle install --standalone
systemctl restart httpd

# add unprivileged user we can run commands on the system as
useradd user
passwd user

# set up an ssh key that can access the cosmos box
sudo -u user ssh-keygen -t rsa -C user -f /home/user/.ssh/id_rsa
sudo -u user ssh-copy-id op1@10.100.$TEAM.5

# add our credentials to the user to give to teams later if they don't solve it
cp /root/finals-challenge-0/backdoor/id_rsa.pub /home/user/.ssh/authorized_keys
chown user:user /home/user/.ssh/authorized_keys

# set up the setuid command runner
cp /root/finals-challenge-0/runner/runner /usr/local/bin/
chown user:user /usr/local/bin/runner
chmod u+s /usr/local/bin/runner

# add a small hint in the user's home directory
cat << EOF > /home/user/stuff
Set up this temporary account so I can access the Cosmos server from home
in case there's a pandemic. Never know what crazy stuff could happen with
Y2K right around the corner. I'll change the username and password later.
EOF
chown user:user /home/user/stuff

# add intended commands for teams to run in the user's bash history
cat << EOF > /home/user/.bash_history
vim stuff
ssh-keygen -t rsa
ssh-copy-id op1@10.100.$TEAM.5
ssh op1@10.100.$TEAM.5
EOF
chown user:user /home/user/.bash_history

# populate the database
#psql -U launchsip -h localhost -W launchsip -f /root/finals-challenge-0/make-clean.sql
