#! /usr/bin/python
import sys
import os
import codecs
from xml.dom.minidom import parse
from bs4 import BeautifulSoup

def getText(nodelist):
	rc = []
	for node in nodelist:
		if node.nodeType == node.TEXT_NODE:
			rc.append(node.data)
	return ''.join(rc)

# returns a dictionary from Cinder class name to file path
def getSymbolToFileMap( path ):	
	f = open( path )
	tagDom = parse( f )
	
	classDict = {}
	compounds = tagDom.getElementsByTagName( "compound" )
	for compound in compounds:
		if compound.getAttribute( "kind" ) == "class":
			className = getText( compound.getElementsByTagName("name")[0].childNodes )
			filePath = getText( compound.getElementsByTagName("filename")[0].childNodes )
			classDict[className] = filePath

	return classDict

# searches the classMap for a given symbol, prepending cinder:: if not found as-is
def getFilePathForSymbol( classMap, className ):
	if className.find( "ci::" ) == 0:
		return classMap[className.replace( "ci::", "cinder::" )]
	elif className in classMap:
		return classMap[className]
	elif ("cinder::" + className) in classMap:
		return classMap["cinder::"+className]
	else:
		return None

# replace all <d> tags in HTML string 'html' based on Doxygen symbols
# in 'symbolMap', making the paths relative to 'doxyHtmlPath'
def processHtml( html, symbolMap, doxyHtmlPath ):
	soup = BeautifulSoup( html )
	for link in soup.find_all('d'):
		searchString = ''
		if link.get( 'dox' ) != None:
			searchString = link.get( 'dox' )
		else:
			searchString = link.contents[0]
		fileName = getFilePathForSymbol( symbolMap, searchString )
		if fileName == None:
			print "*** Error: Could not find Doxygen tag for " + searchString
		else:
			link.name = 'a'
			link['href'] = doxyHtmlPath + fileName
			print link
			
	return soup.prettify()

def processHtmlFile( inPath, symbolMap, doxygenHtmlPath, outPath ):
	outFile = codecs.open( outPath, "w", "utf-8" )
	outFile.write( processHtml( codecs.open( inPath, "r", "utf-8" ), symbolMap, doxygenHtmlPath ) )

doxygenHtmlPath = os.path.dirname( os.path.realpath(__file__) ) + os.sep + 'html' + os.sep
if len( sys.argv ) == 2: # default; generate all docs
	classMap = getSymbolToFileMap( "doxygen/cinder.tag" )
elif len( sys.argv ) == 3:
	symbolMap = getSymbolToFileMap( "doxygen/cinder.tag" )
	processHtmlFile( sys.argv[1], symbolMap, doxygenHtmlPath, sys.argv[2] )
else:
	print "Unknown usage"