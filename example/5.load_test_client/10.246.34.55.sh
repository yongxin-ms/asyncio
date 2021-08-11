#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
ulimit -c unlimited

$basepath/../bin/LoadTestClient 10.246.34.55 9000 10
