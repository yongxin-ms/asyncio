#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
ulimit -c unlimited

$basepath/../bin/LoadTestServer 9000
