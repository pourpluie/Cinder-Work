#pragma once

#include "cinder/gl/gl.h"
#include <memory>
#include <vector>

namespace cinder { namespace gl {

typedef std::shared_ptr<class Vao> VaoRef;

class Vao : public std::enable_shared_from_this<Vao> {
  public:
	static VaoRef		create();
	~Vao();
	
	void				bind();
	void				unbind() const;

	GLuint				getId() const { return mId; }
	
  protected:
	Vao();

	// Does the actual "work" of binding the VAO; called by Context
	static void		bindImpl( GLuint id, class Context *context );
	static void		unbindImpl( class Context *context );
	// Causes Context to reflect any state cache invalidations due to binding/unbinding a VAO
	static void		invalidateContext( class Context *context );
	
	GLuint							mId;
	friend class Context;
};
	
} }
