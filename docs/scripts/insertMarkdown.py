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


def create_updated_rst_doc():
    rst_buffer = []
    rst_fp = open('../ADUVC/ADUVC.rst', 'r')
    rst_lines = rst_fp.readlines()
    rst_fp.close()
    release_fp = open('../../RELEASE.md', 'r')
    release_lines = release_fp.readlines()
    release_fp.close()
    done_release = True
    in_release = False
    for line in rst_lines:
        if ':name: release-notes-1' in line:
            in_release = True
            done_release = False
            rst_buffer.append(line)
        elif in_release and not done_release:
            rel_notes = False
            for rel_line in release_lines:
                if rel_line.startswith('<!--RELEASE START-->'):
                    rel_notes = True
                elif rel_notes:
                    if rel_line.startswith('R') and not rel_line.startswith('Release Notes'):
                        print(rel_line)
                        date = rel_line.split('(')[1].split(')')[0].split('-')
                        rel_name = 'r{}-{}-{}-{}'.format(rel_line[1:4], date[0], date[1].lower(), date[2])
                        rst_buffer.append('.. rubric:: {}\n   :name: {}\n'.format(rel_line.strip(), rel_name))
                    elif rel_line.startswith('*'):
                        rst_buffer.append('- ' + rel_line[1:] + '\n')
                    elif rel_line.strip().startswith('*'):
                        rst_buffer.append('   - ' + rel_line.strip()[1:] + '\n')

                    else:
                        rst_buffer.append(line)
            in_release=False
        elif line.startswith('..raw:: html') and not done_release:
            done_release=True
            rst_buffer.append(line)
        elif not in_release and done_release:
            rst_buffer.append(line)

    return rst_buffer


    

# Main driver function
def update_release_notes():
    fix_header_sizes()
    create_updated_doc()
    rst_doc = create_updated_rst_doc()
    new_rst_fp = open('tempRST.rst', 'w')
    for line in rst_doc:
        new_rst_fp.write(line)
    new_rst_fp.close()
    os.remove("../index.html")
    os.remove("output.html")
    os.rename("../tempIndex.html", "../index.html")


#update_release_notes()
#print(create_updated_rst_doc())

rst_doc = create_updated_rst_doc()
new_rst_fp = open('tempRST.rst', 'w')
for line in rst_doc:
    new_rst_fp.write(line)
new_rst_fp.close()

