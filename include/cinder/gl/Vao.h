#pragma once

#include "cinder/gl/gl.h"
#include <memory>
#include <vector>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;

class Vao {
  public:
	enum : GLuint
	{
		ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_COLOR, ATTRIB_TEXCOORD
	};
	
	/////////////////////////////////////////////////////////////////
	
	class Attribute
	{
	public:
		Attribute( GLuint index = 0, GLint size = 4, GLenum type = GL_FLOAT,
				  GLboolean normalized = GL_FALSE, GLsizei stride = 0,
				  const GLvoid* offset = 0 );
		~Attribute();
		
		void						buffer();
		void						enable( bool enabled = true );
		
		GLuint						getIndex() const;
		GLboolean					getNormalized() const;
		const GLvoid*				getOffset() const;
		GLint						getSize() const;
		GLsizei						getStride() const;
		GLenum						getType() const;
		
		void						setIndex( GLuint value );
		void						setNormalized( GLboolean value );
		void						setOffset( const GLvoid* value );
		void						setSize( GLint value );
		void						setStride( GLsizei value );
		void						setType( GLenum value );
	protected:
		
		GLuint						mIndex;
		GLboolean					mNormalized;
		const GLvoid*				mOffset;
		GLint						mSize;
		GLsizei						mStride;
		GLenum						mType;
		
		friend class				Vao;
	};

	/////////////////////////////////////////////////////////////////
	
	static VaoRef					create();
	~Vao();
	
	void							bind() const;
	void							unbind() const;

	GLuint							getId() const { return mId; }

	void	vertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );
	void	enableVertexAttribArray( GLuint index );
	void	bindBuffer( const VboRef &vbo );
	
	
	
	
	
	
	
	
	void							addAttribute( const Attribute& attr );
	std::vector<Attribute>&			getAttributes();
	const std::vector<Attribute>&	getAttributes() const;
	void							removeAttribute( GLuint index );
	
  protected:
	Vao();

	GLuint							mBoundElementArrayBuffer, mBoundArrayBuffer;
	
	std::vector<Attribute>			mAttributes;
	GLuint							mId;
	friend class Context;
};
	
} }
