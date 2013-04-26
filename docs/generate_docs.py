#! /usr/bin/python
import sys
import os
import codecs
import shutil
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
			print "   ** Warning: Could not find Doxygen tag for " + searchString
		else:
			link.name = 'a'
			link['href'] = doxyHtmlPath + fileName
			
	return soup.prettify()

def processHtmlFile( inPath, symbolMap, doxygenHtmlPath, outPath ):
	outFile = codecs.open( outPath, "w", "ISO-8859-1" )
	outFile.write( processHtml( codecs.open( inPath, "r", "ISO-8859-1" ), symbolMap, doxygenHtmlPath ) )

# walks all files in 'htmlSourceDir' and process in .html, copies/outputs to htmlOutDir 
def processHtmlDir( htmlSourceDir, symbolMap, doxygenHtmlPath ):
	for root, subFolders, files in os.walk( htmlSourceDir ):
		for fileName in files:
			inPath = os.path.join( root, fileName )
			outPath = os.path.join( doxygenHtmlPath, fileName )
			if os.path.splitext( fileName )[1] == '.html':
				print "[ " + fileName + "->" + outPath
				processHtmlFile( inPath, symbolMap, doxygenHtmlPath, outPath )
			else:
				shutil.copyfile( inPath, outPath )

doxygenHtmlPath = os.path.dirname( os.path.realpath(__file__) ) + os.sep + 'html' + os.sep
htmlSourcePath = os.path.dirname( os.path.realpath(__file__) ) + os.sep + 'htmlsource' + os.sep

if len( sys.argv ) == 1: # default; generate all docs
	symbolMap = getSymbolToFileMap( "doxygen/cinder.tag" )
	processHtmlDir( htmlSourcePath, symbolMap, doxygenHtmlPath )
elif len( sys.argv ) == 3:
	symbolMap = getSymbolToFileMap( "doxygen/cinder.tag" )
	processHtmlFile( sys.argv[1], symbolMap, doxygenHtmlPath, sys.argv[2] )
else:
	print "Unknown usage"