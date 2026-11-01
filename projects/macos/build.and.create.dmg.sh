#!/bin/bash
set -euo pipefail
./build.app.sh $1
./create.dmg.sh $1
