#!/bin/bash

if [ ! -d "./pienv" ]
then
    python3 -m venv pienv
fi

source ./pienv/bin/activate
pushd app-api/python && pip3 install . --user && pip3 install -r requirements.txt && popd

mv ./pienv ../

echo -e "Pienv now contains all requirements for python mission apps dev."