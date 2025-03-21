#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"

pydm -m "P=MYIOC:" ./tempMonApp/op/pydm/cpu_temp.ui
