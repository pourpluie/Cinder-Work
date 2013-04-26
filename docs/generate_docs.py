#! /usr/bin/python
import sys
from xml.dom.minidom import parse

def getText(nodelist):
	rc = []
	for node in nodelist:
		if node.nodeType == node.TEXT_NODE:
			rc.append(node.data)
	return ''.join(rc)

# returns a dictionary from Cinder class name to file path
def getClassToFileMap( path ):	
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

# returns a list of [name,category]; a category is a [(phrase,cinder symbol)]
def parseClassLayout( path ):
	result = []
	curCategory = []
	f = open( path )
	lines = f.readlines()
	for line in lines:
		words = line.strip().split()
		if len(words) == 0:
			continue
		elif words[0].find( '[' ) == 0: # new category
			if len(curCategory) > 0:
				result.append( curCategory )
			curCategory = [line.strip().replace('[','').replace(']',''), []]
		else:
			phrase = words[0]
			for x in range( 1, len(words) - 1 ):
				phrase += ' ' + words[x]
			symbol = words[len(words)-1]
			curCategory[1].append( (phrase, symbol) )
	result.append( curCategory )
	return result

# searches the classMap for a given symbol, prepending cinder:: if not found as-is
def getFilePathFromClassMap( classMap, className ):
	if className.find( "ci::" ) == 0:
		return classMap[className.replace( "ci::", "cinder::" )]
	elif className in classMap:
		return classMap[className]
	elif ("cinder::" + className) in classMap:
		return classMap["cinder::"+className]
	else:
		print "Warning: could not find " + className
		return ""

# <ul>
#   <li class="category">{category name}</li>
#   <li><ul>
#		<li class="className"><a href="{file path}">{class name}</li>
#	</ul></li>
def generateIndex( classMap, classLayout, filePathPrefix, out ):
	out.write( "<html>" )
	out.write( "<ul>" )
	for category in classLayout:
		out.write( '<li class="categoryName">' + category[0] + '</li>' )
		out.write( '<ul>' )
		for (phrase, className) in category[1]:
			filePath = getFilePathFromClassMap( classMap, className )
			out.write( '\t<li><a href="' + filePathPrefix + filePath + '">' + phrase + '</a></li>' )
		out.write( '</ul>' )
	out.write( '</ul>' )
	out.write( '</html>' )

classMap = getClassToFileMap( "doxygen/cinder.tag" )
classLayout = parseClassLayout( "class_layout.txt" )
generateIndex( classMap, classLayout, 'html/', sys.argv[2] )
