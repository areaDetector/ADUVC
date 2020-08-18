# Python script that replaces the old release notes with notes converted from markdown into the docs
#
# Author: Jakub Wlodek
# Created on: January 10, 2019
#
# Copyright(c): Brookhaven National Laboratory 2018-2019
#

import os


# function that changes <h2> tags to <h4> tags
def fix_header_sizes():
    with open("output.html") as f:
        newText = f.read().replace('h2', 'h4')
    
    with open("output.html", "w") as f:
        f.write(newText)


# function that inserts new release notes
def insert_new_notes(tempIndex):
    newNotes = open("output.html", "r")
    line = newNotes.readline()
    line_counter = 0
    while line:
        if line_counter > 3:
            tempIndex.write(line)
        line_counter = line_counter + 1
        line = newNotes.readline()
    newNotes.close()


# function that copies unchanged html from file
def create_updated_doc():
    htmlDoc = open("../index.html", "r")
    tempIndex = open("../tempIndex.html", "w")
    line = htmlDoc.readline()
    isReleases = False
    while line:
        if isReleases == False:
            tempIndex.write(line)
        if "<!--RELEASE START-->" in line:
            isReleases = True
        elif "<!--RELEASE END-->" in line:
            insert_new_notes(tempIndex)
            isReleases = False
            tempIndex.write(line)
        line = htmlDoc.readline()
    htmlDoc.close()
    tempIndex.close()


# Main driver function
def update_release_notes():
    fix_header_sizes()
    create_updated_doc()
    os.remove("../index.html")
    os.remove("output.html")
    os.rename("../tempIndex.html", "../index.html")


update_release_notes()
