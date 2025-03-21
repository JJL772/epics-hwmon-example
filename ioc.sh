#!/usr/bin/env bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")"
cd iocs/tempMonExampleIoc/iocBoot/sioc-temp-mon-test

if [ -z "$EPICS_HOST_ARCH" ]; then
    echo "EPICS_HOST_ARCH not defined :("
    exit 1
fi

$DEBUGGER "../../bin/$EPICS_HOST_ARCH/tempMonIoc" st.cmd
