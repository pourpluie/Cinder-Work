#include "cinder/gl/Shader.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;

namespace cinder { namespace gl {

Shader::Shader()


/////////////////////////////////////////////////////////////////

#if 0
	
Shader::UniformOptions::UniformOptions()
: mColorEnabled( false ), mFogEnabled( false ), mLightingModel( Shader::LightingModel::NONE ),
mMaterialEnabled( false ), mNumLights( 0 ), mPrecision( Shader::Precision::MEDIUM ), mTextureEnabled( false )
{
}
		
Shader::UniformOptions& Shader::UniformOptions::enableColor( bool enabled )
{
	mColorEnabled = enabled;
	return *this;
}

bool Shader::UniformOptions::isColorEnabled() const
{
	return mColorEnabled;
}

Shader::UniformOptions& Shader::UniformOptions::enableFog( bool enabled )
{
	mFogEnabled = enabled;
	return *this;
}

bool Shader::UniformOptions::isFogEnabled() const
{
	return mFogEnabled;
}
	
Shader::UniformOptions& Shader::UniformOptions::enableMaterial( bool enabled )
{
	mMaterialEnabled = enabled;
	return *this;
}

bool Shader::UniformOptions::isMaterialEnabled() const
{
	return mMaterialEnabled;
}

Shader::UniformOptions& Shader::UniformOptions::enableTexture( bool enabled )
{
	mTextureEnabled = enabled;
	return *this;
}

bool Shader::UniformOptions::isTextureEnabled() const
{
	return mTextureEnabled;
}


Shader::UniformOptions&	Shader::UniformOptions::setLightingModel( Shader::LightingModel lm )
{
	mLightingModel = lm;
	return *this;
}

Shader::LightingModel Shader::UniformOptions::getLightingModel() const
{
	return mLightingModel;
}

Shader::UniformOptions& Shader::UniformOptions::setNumLights( size_t numLights )
{
	mNumLights = numLights;
	return *this;
}

size_t Shader::UniformOptions::getNumLights() const
{
	return mNumLights;
}
	
Shader::UniformOptions&	Shader::UniformOptions::setPrecision( Shader::Precision p )
{
	mPrecision = p;
	return *this;
}

Shader::Precision Shader::UniformOptions::getPrecision() const
{
	return mPrecision;
}

bool Shader::UniformOptions::operator==( const Shader::UniformOptions& rhs ) const
{
	return mColorEnabled	== rhs.mColorEnabled	&&
		mFogEnabled			== rhs.mFogEnabled		&&
		mLightingModel		== rhs.mLightingModel	&&
		mMaterialEnabled	== rhs.mMaterialEnabled	&&
		mNumLights			== rhs.mNumLights		&&
		mPrecision			== rhs.mPrecision		&&
		mTextureEnabled		== rhs.mTextureEnabled;
}
	
bool Shader::UniformOptions::operator!=( const Shader::UniformOptions& rhs ) const
{
	return !( *this == rhs );
}

bool Shader::UniformOptions::operator<( const Shader::UniformOptions& rhs ) const
{
	return *this != rhs;
}

/////////////////////////////////////////////////////////////////	
	
ShaderRef Shader::create( const UniformOptions& options )
{
	return ShaderRef( new Shader( options ) );
}

Shader::Shader( const UniformOptions& options )
{
	// We're goind to build this shader in three steps:
	// 1) Write the GlslProg using the UniformOptions
	// 2) Acquire all uniform locations
	// 3) Set all uniforms with default values
	
	mUniformOptions = options;
	string precision;
	switch ( mUniformOptions.getPrecision() )
	{
		case Precision::LOW:
			precision = "lowp";
			break;
		case Precision::MEDIUM:
			precision = "mediump";
			break;
		case Precision::HIGH:
			precision = "highp";
			break;
	}
	size_t numLights = mUniformOptions.getNumLights();
	bool lightingEnabled = numLights > 0 && mUniformOptions.getLightingModel() != LightingModel::NONE;
	
	// 1) Write the GlslProg using the UniformOptions
	
	// VERTEX SHADER
	// Nothing fancy. Just transform position and pass attributes.
	
	mVertexString  = "";
	mVertexString += "attribute vec4 aPosition;\n";
	mVertexString += "attribute vec4 aNormal;\n";
	mVertexString += "attribute vec4 aColor;\n";
	mVertexString += "attribute vec4 aTexCoord;\n";
	mVertexString += "\n";
	mVertexString += "uniform mat4 uModelViewProjection;\n";
	mVertexString += "\n";
	mVertexString += "varying " + precision + " vec4 vColor;\n";
	mVertexString += "varying " + precision + " vec4 vNormal;\n";
	mVertexString += "varying " + precision + " vec4 vPosition;\n";
	mVertexString += "varying " + precision + " vec4 vTexCoord;\n";
	mVertexString += "\n";
	mVertexString += "void main( void ) {\n";
	mVertexString += "\tvColor = aColor;\n";
	mVertexString += "\tvNormal = aNormal;\n";
	mVertexString += "\tvPosition = uModelViewProjection * aPosition;\n";
	mVertexString += "\tvTexCoord = aTexCoord;\n";
	mVertexString += "\t\n";
	mVertexString += "\tgl_Position = vPosition;\n";
	mVertexString += "}\n";
	
	
	// FRAGMENT SHADER
	
	mFragmentString = "";
	
	
	// TODO Constants -- will override uniforms.
	// eg, constants for attenuation will be used instead
	// of values form light structure. This may mean removing
	// properties from structures to reduce overhead. 
	// e.g., mFragmentString += "const " + precision + " float kAttenuationConstant = " + toString( mConstantOptions.phongModel().getConstantAttenuation() ) + ";\n";
	
	// Fog structure
	if ( mUniformOptions.isFogEnabled() ) {
		mFragmentString += "struct Fog {\n";
		mFragmentString += "\t" + precision + " vec4 color;\n";
		mFragmentString += "\t" + precision + " float density;\n";
		mFragmentString += "\t" + precision + " float end;\n";
		mFragmentString += "\t" + precision + " float scale;\n";
		mFragmentString += "\t" + precision + " float start;\n";
		mFragmentString += "};\n";
		mFragmentString += "\n";
	}
	
	// Light structure
	if ( lightingEnabled ) {
		mFragmentString += "struct Light {\n";
		mFragmentString += "\t" + precision + " int type;\n";
		mFragmentString += "\t" + precision + " vec4 colorAmbient;\n";
		mFragmentString += "\t" + precision + " vec4 colorDiffuse;\n";
		mFragmentString += "\t" + precision + " vec4 colorSpecular;\n";
		mFragmentString += "\t" + precision + " float attenuationConstant;\n";
		mFragmentString += "\t" + precision + " float attenuationLinear;\n";
		mFragmentString += "\t" + precision + " float attenuationQuadratic;\n";
		mFragmentString += "\t" + precision + " float shine;\n";
		mFragmentString += "\t" + precision + " vec3 direction;\n";
		mFragmentString += "\t" + precision + " vec3 position;\n";
		mFragmentString += "};\n";
		mFragmentString += "\n";
	}
	
	// Material structure
	if ( mUniformOptions.isMaterialEnabled() ) {
		mFragmentString += "struct Material {\n";
		mFragmentString += "\t" + precision + " float ambient;\n";
		mFragmentString += "\t" + precision + " vec4 color;\n";
		mFragmentString += "\t" + precision + " float diffuse;\n";
		mFragmentString += "\t" + precision + " float emissive;\n";
		mFragmentString += "\t" + precision + " float specular;\n";
		mFragmentString += "};\n";
		mFragmentString += "\n";
	}
	
	// Optional uniforms
	if ( mUniformOptions.isColorEnabled() ) {
		mFragmentString += "uniform " + precision + " vec4 uColor;\n";
	}
	if ( mUniformOptions.isFogEnabled() ) {
		mFragmentString += "uniform Fog uFog;\n";
	}
	if ( lightingEnabled ) {
		mFragmentString += "uniform " + precision + " vec3 uEyePoint;\n";
		mFragmentString += "uniform Light uLight[ " + toString( numLights ) + " ];\n";
	}
	if ( mUniformOptions.isMaterialEnabled() ) {
		mFragmentString += "uniform Material uMaterial;\n";
	}
	if ( mUniformOptions.isTextureEnabled() ) {
		mFragmentString += "uniform sampler2D uTextureId;\n";
	}
	mFragmentString += "\n";
	
	// Attributes
	mFragmentString += "varying " + precision + " vec4 vColor;\n";
	mFragmentString += "varying " + precision + " vec4 vNormal;\n";
	mFragmentString += "varying " + precision + " vec4 vPosition;\n";
	mFragmentString += "varying " + precision + " vec4 vTexCoord;\n";
	mFragmentString += "\n";
	
	// Begin program
	mFragmentString += "void main( void ) {\n";
	
	// d	= distance to light
	// Ix	= result color
	// Lx	= light color
	// Ax	= ambient color
	// Dx	= diffuse color
	// Sx	= specular color
	// Ka	= ambient coefficient
	// Kd	= diffuse coefficient
	// Ke	= emissive coefficient
	// Ks	= specular coefficient
	// Att	= attenuation coefficient
	// n	= shine
	// N	= surface normal
	// L	= light vector
	// r	= range
	// R	= reflection vector
	// V	= view vector
	// l	= Lambert term

	// Albedo (color)
	mFragmentString +=	"\t" + precision + " vec4 Ix = vColor;\n";
	if ( mUniformOptions.isColorEnabled() ) {
		//mFragmentString +=	"\tIx *= uColor;\n";
	}
	if ( mUniformOptions.isMaterialEnabled() ) {
		//mFragmentString +=	"\tIx *= uMaterial.color;\n";
	}
	if ( mUniformOptions.isTextureEnabled() ) {
		mFragmentString +=	"\tIx *= texture2D( uTextureId, vTexCoord.st );\n";
	}
	
	// TODO -- handle light types (directional, point, spotlight)
	
	// Lighting -- TODO ignore unused values (eg, no attenuation for blinn or Lambert, etc)
	if ( lightingEnabled ) {
		mFragmentString +=	"\tfor ( lowp int i = 0; i < " + toString( numLights ) + "; ++i ) {\n";
		mFragmentString +=	"\t\t" + precision + " float d = distance( uEyePoint, vPosition.xyz );\n";
		mFragmentString +=	"\n";
		mFragmentString +=	"\t\t" + precision + " float Att = 1.0 / ( uLight[ i ].attenuationConstant + uLight[ i ].attenuationLinear * d + uLight[ i ].attenuationQuadratic * ( d * d ) );\n";
		mFragmentString +=	"\t\t" + precision + " float n = uLight[ i ].shine;\n";
		mFragmentString +=	"\n";
		mFragmentString +=	"\t\t" + precision + " vec3 N = vNormal.xyz;\n";
		mFragmentString +=	"\t\t" + precision + " vec3 L = normalize( uLight[ i ].position - vPosition.xyz );\n";
		mFragmentString +=	"\t\t" + precision + " vec3 R = normalize( -reflect( L, N ) );\n";
		mFragmentString +=	"\t\t" + precision + " vec3 V = normalize( -uEyePoint );\n";
		mFragmentString +=	"\t\t" + precision + " float l = max( dot( N, L ), 0.0 );\n";
		mFragmentString +=	"\n";
		mFragmentString +=	"\t\t" + precision + " vec4 Ax = uLight[ i ].colorAmbient;\n";
		mFragmentString +=	"\t\t" + precision + " vec4 Dx = uLight[ i ].colorDiffuse;\n";
		mFragmentString +=	"\t\t" + precision + " vec4 Sx = uLight[ i ].colorSpecular;\n";
		mFragmentString +=	"\n";
		mFragmentString +=	"\t\t" + precision + " float Ke = uMaterial.emissive;\n";
		mFragmentString +=	"\t\t" + precision + " float Ka = uMaterial.ambient + Ke;\n";
		mFragmentString +=	"\t\t" + precision + " float Kd = uMaterial.diffuse + Ke;\n";
		mFragmentString +=	"\t\t" + precision + " float Ks = uMaterial.specular + Ke;\n";
		mFragmentString +=	"\n";
		
		switch ( mUniformOptions.getLightingModel() ) {
			case LightingModel::BLINN:
				mFragmentString +=	"\t\t" + precision + " vec4 Lx = Ax * Ka * Dx + Sx * Ks * pow( max( dot( R, V ), 0.0 ), n );\n";
				break;
			case LightingModel::LAMBERT:
				mFragmentString +=	"\t\t" + precision + " vec4 Lx = Ax * Ka * Dx;\n";
				break;
			case LightingModel::PHONG:
				mFragmentString +=	"\t\t" + precision + " vec4 Lx = Ax * Ka * Dx + Att * ( Dx * Kd * l + Sx * Ks * pow( max( dot( R, V ), 0.0 ), n ) );\n";
				break;
			default:
				break;
		}
				
		mFragmentString +=	"\t\tIx += Lx * l;\n";
		mFragmentString +=	"\t}\n";
		mFragmentString +=	"\n";
	}
	
	// Fog
	
	mFragmentString += "\tgl_FragColor = Ix;\n";
	mFragmentString += "}\n";
	
	//app::console() << mVertexString << endl;
	//app::console() << mFragmentString << endl;
	
	try {
		mGlslProg = gl::GlslProg::create( mVertexString.c_str(), mFragmentString.c_str() );
	} catch ( gl::GlslProgCompileExc ex ) {
		app::console() << ex.what() << endl;
	}
	
	// 2) Acquire all uniform locations
	
	mHnd = mGlslProg->getHandle();
	
	mUniformLocModelViewProjectionMatrix	= glGetUniformLocation( mHnd, "uModelViewProjection" );
	
	if ( mUniformOptions.isColorEnabled() ) {
		mUniformLocColor					= glGetUniformLocation( mHnd, "uColor" );
	}
	if ( mUniformOptions.isTextureEnabled() ) {
		mUniformLocTextureId				= glGetUniformLocation( mHnd, "uTextureId" );
	}
	
	if ( mUniformOptions.isFogEnabled() ) {
		mUniformLocFogColor					= glGetUniformLocation( mHnd, "uFog.color" );
		mUniformLocFogDensity				= glGetUniformLocation( mHnd, "uFog.density" );
		mUniformLocFogEnd					= glGetUniformLocation( mHnd, "uFog.end" );
		mUniformLocFogScale					= glGetUniformLocation( mHnd, "uFog.scale" );
		mUniformLocFogStart					= glGetUniformLocation( mHnd, "uFog.start" );
	}
	
	if ( numLights > 0 ) {
		mUniformLocEyePoint						= glGetUniformLocation( mHnd, "uEyePoint" );		
		for ( size_t i = 0; i < numLights; ++i ) {
			string index = toString( i );

			mUniformLocLightAttenuationConstant[ i ]	= glGetUniformLocation( mHnd, ( "uLight[" + index + "].attenuationConstant" ).c_str() );
			mUniformLocLightAttenuationLinear[ i ]		= glGetUniformLocation( mHnd, ( "uLight[" + index + "].attenuationLinear" ).c_str() );
			mUniformLocLightAttenuationQuadratic[ i ]	= glGetUniformLocation( mHnd, ( "uLight[" + index + "].attenuationQuadratic" ).c_str() );
			mUniformLocLightColorAmbient[ i ]			= glGetUniformLocation( mHnd, ( "uLight[" + index + "].colorAmbient" ).c_str() );
			mUniformLocLightColorDiffuse[ i ]			= glGetUniformLocation( mHnd, ( "uLight[" + index + "].colorDiffuse" ).c_str() );
			mUniformLocLightColorSpecular[ i ]			= glGetUniformLocation( mHnd, ( "uLight[" + index + "].colorSpecular" ).c_str() );
			mUniformLocLightDirection[ i ]				= glGetUniformLocation( mHnd, ( "uLight[" + index + "].direction" ).c_str() );
			mUniformLocLightPosition[ i ]				= glGetUniformLocation( mHnd, ( "uLight[" + index + "].position" ).c_str() );
			mUniformLocLightShine[ i ]					= glGetUniformLocation( mHnd, ( "uLight[" + index + "].shine" ).c_str() );
			mUniformLocLightType[ i ]					= glGetUniformLocation( mHnd, ( "uLight[" + index + "].type" ).c_str() );
		}
	}
	
	if ( mUniformOptions.isMaterialEnabled() ) {
		mUniformLocMaterialAmbient			= glGetUniformLocation( mHnd, "uMaterial.ambient" );
		mUniformLocMaterialColor			= glGetUniformLocation( mHnd, "uMaterial.color" );
		mUniformLocMaterialDiffuse			= glGetUniformLocation( mHnd, "uMaterial.diffuse" );
		mUniformLocMaterialEmissive			= glGetUniformLocation( mHnd, "uMaterial.emissive" );
		mUniformLocMaterialSpecular			= glGetUniformLocation( mHnd, "uMaterial.specular" );
	}
	
	// 3) Set all uniforms with default values
	
	mColor		= ColorAf::white();
	mEyePoint	= Vec3f::zero();
	mTextureId	= 0;
	mModelViewProjectionMatrix.setToIdentity();
	
	if ( numLights > 0 ) {
		Light light;
		for ( size_t i = 0; i < numLights; ++i ) {
			mLights.push_back( light );
			mLightAttenuationConstant.push_back( light.getConstantAttenuation() );
			mLightAttenuationLinear.push_back( light.getLinearAttenuation() );
			mLightAttenuationQuadratic.push_back( light.getQuadraticAttenuation() );
			mLightColorAmbient.push_back( light.getAmbient() );
			mLightColorDiffuse.push_back( light.getDiffuse() );
			mLightColorSpecular.push_back( light.getSpecular() );
			mLightDirection.push_back( light.getDirection() );
			mLightPosition.push_back( light.getPosition() );
			mLightShine.push_back( light.getShine() );
			mLightType.push_back( light.getType() );
		}
	}
	
	update();
}
	
Shader::~Shader()
{
	mLightAttenuationConstant.clear();
	mLightAttenuationLinear.clear();
	mLightAttenuationQuadratic.clear();
	mLightColorAmbient.clear();
	mLightColorDiffuse.clear();
	mLightColorSpecular.clear();
	mLightDirection.clear();
	mLightPosition.clear();
	mLightShine.clear();
	mLightType.clear();
}

void Shader::bind()
{
	mGlslProg->bind();
}

void Shader::unbind()
{
	mGlslProg->unbind();
}
	
void Shader::update()
{	
	mGlslProg->bind();
	
	if ( mUniformOptions.isColorEnabled() ) {
		glUniform4f( mUniformLocColor, mColor.r, mColor.g, mColor.b, mColor.a );
	}
	
	glUniformMatrix4fv( mUniformLocModelViewProjectionMatrix, 1, false, mModelViewProjectionMatrix.m );
	
	if ( mUniformOptions.isTextureEnabled() ) {
		glUniform1i( mUniformLocTextureId, mTextureId );
	}
	
	if ( mUniformOptions.isFogEnabled() ) {
		ColorAf c = mFog.getColor();
		glUniform4f( mUniformLocFogColor, c.r, c.g, c.b, c.a );
		glUniform1f( mUniformLocFogDensity, mFog.getDensity() );
		glUniform1f( mUniformLocFogEnd, mFog.getEnd() );
		glUniform1f( mUniformLocFogScale, mFog.getScale() );
		glUniform1f( mUniformLocFogStart, mFog.getStart() );
	}
	
	size_t numLights = mUniformOptions.getNumLights();
	if ( numLights > 0 ) {
		glUniform3f( mUniformLocEyePoint, mEyePoint.x, mEyePoint.y, mEyePoint.z );
		
		for ( size_t i = 0; i < numLights; ++i ) {

			glUniform1f( mUniformLocLightAttenuationConstant[ i ], mLightAttenuationConstant[ i ] );
			glUniform1f( mUniformLocLightAttenuationLinear[ i ], mLightAttenuationLinear[ i ] );
			glUniform1f( mUniformLocLightAttenuationQuadratic[ i ], mLightAttenuationQuadratic[ i ] );
			
			ColorAf a = mLightColorAmbient[ i ];
			ColorAf d = mLightColorDiffuse[ i ];
			ColorAf s = mLightColorSpecular[ i ];
			glUniform4f( mUniformLocLightColorAmbient[ i ], a.r, a.g, a.b, a.a );
			glUniform4f( mUniformLocLightColorDiffuse[ i ], d.r, d.g, d.b, d.a );
			glUniform4f( mUniformLocLightColorSpecular[ i ], s.r, s.g, s.b, s.a );
			
			Vec3f dir = mLightDirection[ i ];
			Vec3f pos = mLightPosition[ i ];
			glUniform3f( mUniformLocLightDirection[ i ], dir.x, dir.y, dir.z );
			glUniform3f( mUniformLocLightPosition[ i ], pos.x, pos.y, pos.z );
			
			glUniform1f( mUniformLocLightShine[ i ], mLightShine[ i ] );
			
			glUniform1i( mUniformLocLightType[ i ], mLightType[ i ] );
		}
	}
	
	if ( mUniformOptions.isMaterialEnabled() ) {
		ColorAf c = mMaterial.getColor();
		glUniform1f( mUniformLocMaterialAmbient, mMaterial.getAmbient() );
		glUniform4f( mUniformLocMaterialColor, c.r, c.g, c.b, c.a );
		glUniform1f( mUniformLocMaterialDiffuse, mMaterial.getDiffuse() );
		glUniform1f( mUniformLocMaterialEmissive, mMaterial.getEmissive() );
		glUniform1f( mUniformLocMaterialSpecular, mMaterial.getSpecular() );
	}
	
	mGlslProg->unbind();
}
	
const ColorAf& Shader::getColor() const
{
	return mColor;
}
	
void Shader::setColor( const ColorAf& color )
{
	mColor = color;
}

const Vec3f& Shader::getEyePoint() const
{
	return mEyePoint;
}
	
void Shader::setEyePoint( const Vec3f& v )
{
	mEyePoint = v;
}

const Fog& Shader::getFog() const
{
	return mFog;
}

void Shader::setFog( const Fog& fog )
{
	mFog = fog;
}

GlslProgRef Shader::getGlslProg() const
{
	return mGlslProg;
}

const Light& Shader::getLight( size_t id ) const
{
	return mLights.at( id );
}
	
void Shader::setLight( size_t id, const Light& light )
{
	mLights[ id ]						= light;
	mLightAttenuationConstant[ id ]		= light.getConstantAttenuation();
	mLightAttenuationLinear[ id ]		= light.getLinearAttenuation();
	mLightAttenuationQuadratic[ id ]	= light.getQuadraticAttenuation();
	mLightColorAmbient[ id ]			= light.getAmbient();
	mLightColorDiffuse[ id ]			= light.getDiffuse();
	mLightColorSpecular[ id ]			= light.getSpecular();
	mLightDirection[ id ]				= light.getDirection();
	mLightPosition[ id ]				= light.getPosition();
	mLightShine[ id ]					= light.getShine();
	mLightType[ id ]					= light.getType();
}

const Material& Shader::getMaterial() const
{
	return mMaterial;
}

void Shader::setMaterial( const Material& material )
{
	mMaterial = material;
}

const Matrix44f& Shader::getModelViewProjectionMatrix()
{
	return mModelViewProjectionMatrix;
}

void Shader::setModelViewProjectionMatrix( const Matrix44f& m )
{
	mModelViewProjectionMatrix = m;
}

void Shader::setCamera( const Camera& c )
{
	mEyePoint = c.getEyePoint();
	mModelViewProjectionMatrix = c.getProjectionMatrix() * c.getModelViewMatrix();
}

GLuint Shader::getTextureId() const
{
	return mTextureId;
}

void Shader::setTextureId( GLuint id )
{
	mTextureId = id;
}

const Shader::UniformOptions& Shader::getUniformOptions() const
{
	return mUniformOptions;
}

const string& Shader::getFragmentString() const
{
	return mFragmentString;
}
	
const string& Shader::getVertexString() const
{
	return mVertexString;
}

#endif
	
} }
