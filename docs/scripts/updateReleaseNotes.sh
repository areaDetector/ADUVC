# Simple bash script that converts the RELEASE.md file from markdown to html using the markdown python package
#
# To install this package run:
#
# sudo pip install markdown
#
# Author: Jakub Wlodek
# Created on: January 10, 2019
#

python3 -m markdown ../../RELEASE.md > output.html
python3 insertMarkdown.py
