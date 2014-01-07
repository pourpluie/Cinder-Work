//
//  XFO.h
//  TransformFeedbackObject
//
//  Created by Ryan Bartley on 12/16/13.
//
//

#pragma once

#include "cinder/gl/gl.h"

namespace cinder { namespace gl {

typedef std::shared_ptr<class Xfo> XfoRef;
	
class Xfo : public std::enable_shared_from_this<Xfo>{
public:
	
	struct Layout {
		Layout( GLuint index, gl::VboRef vbo )
		: mIndex( index ), mVbo( vbo )
		{
		}
		
		bool operator==( const Layout &rhs )
		{
			return ( mIndex == rhs.mIndex ) &&
					( mVbo == rhs.mVbo );
		}
		
	private:
		GLuint mIndex;
		gl::VboRef mVbo;
	};
	
	static XfoRef create( bool software );
	virtual ~Xfo() {}
	
	void bind();
	void unbind();
	
	void begin( GLenum primitiveMode );
	void pause();
	void resume();
	void end();
	
	GLuint	getId() const { return mId; }
	bool	getAlreadyBound() const { return mAlreadyBound; }
	
protected:
	Xfo() : mAlreadyBound( false ) {}
	
	virtual void bindImpl( class TestContext *context ) = 0;
	virtual void unbindImpl( class TestContext *context ) = 0;
	
	virtual void setIndex( int index, VboRef vbo ) = 0;
	
	GLuint							mId;
	std::map<GLuint, gl::VboRef>	mBufferBases;
	bool							mAlreadyBound;
	
	friend class TestContext;
};	

} } // cinder // gl