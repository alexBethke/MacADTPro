#!/bin/sh

set -eu

hmount_tool=$1
hcopy_tool=$2
hdel_tool=$3
hattrib_tool=$4
humount_tool=$5
disk_image=$6
application_file=$7
readme_file=$8

"$hmount_tool" "$disk_image"
trap '"$humount_tool" >/dev/null 2>&1 || true' EXIT HUP INT TERM
"$hdel_tool" ':MacADTPro'
"$hcopy_tool" -m "$application_file" ':MacADTPro'
"$hcopy_tool" -t "$readme_file" ':Read Me'
"$hattrib_tool" -t ttro -c ttxt ':Read Me'
"$humount_tool"
trap - EXIT HUP INT TERM
