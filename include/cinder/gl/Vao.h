#pragma once

#include "cinder/gl/gl.h"
#include <memory>
#include <vector>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;

class Vao {
  public:
	static VaoRef		create();
	~Vao();
	
	void				bind() const;
	void				unbind() const;

	GLuint				getId() const { return mId; }
	
  protected:
	Vao();
	
	GLuint							mId;
	friend class Context;
};
	
} }
