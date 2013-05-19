

#include "rh_texture_internal.h"

#define LOGI(...) ((void)printf(__VA_ARGS__))
#define LOGW(...) ((void)printf(__VA_ARGS__))

static int __report_gl_err(const char * file, const char * func, int line) {

	GLenum e;
	int ecount = 0;

	while( (e = glGetError()) != GL_NO_ERROR ) {

		ecount++;
		LOGI("%s:%s:%d gl error %d", file, func, line, e);
	}

	return ecount;
}
#define GL_ERROR() __report_gl_err(__FILE__,__FUNCTION__,__LINE__)

/* code stolen from http://www.opengl.org/archives/resources/features/OGLextensions/ */
static int IsExtensionSupported(const char *extension)
{
  const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;
  extensions = glGetString(GL_EXTENSIONS);
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;) {
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
      if (*terminator == ' ' || *terminator == '\0')
        return 1;
    start = terminator;
  }
  return 0;
}

static GLenum get_gl_compression_enum(int libimg_format) {

	if( (libimg_format & IMG_FMT_COMPONENT_DXT1) == IMG_FMT_COMPONENT_DXT1 )
		return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

	if( (libimg_format & IMG_FMT_COMPONENT_DXT3) == IMG_FMT_COMPONENT_DXT3 )
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

	if( (libimg_format & IMG_FMT_COMPONENT_DXT5) == IMG_FMT_COMPONENT_DXT5 )
		return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

	return -1;
}

static GLint get_uncompressed_internal_format(int libimg_format) {

	if( (libimg_format & IMG_FMT_RGB24) == IMG_FMT_RGB24 )
		return GL_RGB;

	if( (libimg_format & IMG_FMT_RGBA32) == IMG_FMT_RGBA32 )
		return GL_RGBA;

	// other supported formats are GL_ALPHA, GL_LUMINANCE and GL_LUMINANCE_ALPHA

	return -1;
}

static int allocate_texture_array_memory(const struct rhtpak_hdr *header, GLenum target) {

	if( header->format & IMG_FMT_COMPONENT_COMPRESSED) {

		GLenum format = get_gl_compression_enum( header->format );

		GLsizei imageSize = ((header->w+3)/4) * ((header->h+3)/4) * header->depth;

		if( header->format & IMG_FMT_COMPONENT_DXT1 )
			imageSize *= 8;
		else
			imageSize *= 16;


		GL_COMPRESSED_TEX_IMAGE_3D(
			target, 		// TARGET
			0, 			// LEVEL
			format,			// INTERNAL FORMAT
			header->w,		// WIDTH
			header->h, 		// HEIGHT
			header->depth,		// DEPTH
			0, 			// BORDER
			imageSize,		// IMAGE SIZE
			NULL 			// DATA
		);


		return 0;

	} else {

		GLint internal_format = get_uncompressed_internal_format( header->format );

		GL_TEX_IMAGE_3D(
			target,			// TARGET
			0,			// LEVEL
			internal_format,	// INTERNAL FORMAT,
			header->w,		// WIDTH
			header->h, 		// HEIGHT
			header->depth,		// DEPTH
			0, 			// BORDER
			internal_format,	// FORMAT
			GL_UNSIGNED_BYTE,	// TYPE
			NULL			// PIXELS
		);

		return 0;
	}

	return -1;
}

static int allocate_texture_memory(const struct rhtpak_hdr *header, GLenum target) {

	if( header->format & IMG_FMT_COMPONENT_COMPRESSED) {

		GLenum format = get_gl_compression_enum( header->format );

		GLsizei imageSize = ((header->w+3)/4) * ((header->h+3)/4) * header->depth;

		if( header->format & IMG_FMT_COMPONENT_DXT1 )
			imageSize *= 8;
		else
			imageSize *= 16;

		glCompressedTexImage2D(
			target, 	// TARGET
			0, 		// LEVEL
			format,		// INTERNAL FORMAT
			header->w,	// WIDTH
			header->h, 	// HEIGHT
			0, 		// BORDER
			imageSize,	// IMAGE SIZE
			NULL 		// DATA
		);

		return 0;

	} else {

		GLint internal_format = get_uncompressed_internal_format( header->format );

		glTexImage2D(
			target,			// TARGET
			0,			// LEVEL
			internal_format,	// INTERNAL FORMAT,
			header->w,		// WIDTH
			header->h, 		// HEIGHT
			0, 			// BORDER
			internal_format,	// FORMAT
			GL_UNSIGNED_BYTE,	// TYPE
			NULL			// PIXELS
		);

		return 0;
	}

	return -1;
}

static int load_texture_array_data( const struct rhtpak_hdr_tex_data *tex_data, int i, const void * data, int data_len, GLenum target) {

	if( tex_data->format & IMG_FMT_COMPONENT_COMPRESSED) {

		GLenum format = get_gl_compression_enum( tex_data[i].format );

		GL_COMPRESSED_TEX_SUBIMAGE_3D(
				target,		// TARGET
				0,		// LEVEL
				0,		// X OFFSET
				0,		// Y OFFSET
				i,		// Z OFFSET
				tex_data[i].w,	// WIDTH
				tex_data[i].h,	// HEIGHT
				1,		// DEPTH
				format,		// FORMAT
				data_len,	// SIZE
				data		// DATA
		);

		return 0;

	} else {

		GLint internal_format = get_uncompressed_internal_format( tex_data[i].format );

		GL_TEX_SUMBIMAGE_3D(
				target,			// TARGET
				0,			// LEVEL
				0,			// X OFFSET
				0,			// Y OFFSET
				i,			// Z OFFSET
				tex_data[i].w,		// WIDTH
				tex_data[i].h,		// HEIGHT
				1,			// DEPTH
				internal_format,	// FORMAT
				GL_UNSIGNED_BYTE,	// TYPE
				data			// DATA
		);

		return 0;
	}


	return -1;
}

static int load_texture_data( const struct rhtpak_hdr_tex_data *tex_data, int i, const void * data, int data_len, GLenum target) {

	if( tex_data->format & IMG_FMT_COMPONENT_COMPRESSED) {

		GLenum format = get_gl_compression_enum( tex_data[i].format );

		glCompressedTexSubImage2D(
				target,			// TARGET
				0,			// LEVEL
				0,			// X OFFSET
				0,			// Y OFFSET
				tex_data[i].w,		// WIDTH
				tex_data[i].h,		// HEIGHT
				format,			// FORMAT
				data_len,		// SIZE
				data			// DATA
		);

		return 0;

	} else {

		GLint internal_format = get_uncompressed_internal_format( tex_data[i].format );

		glTexSubImage2D(
				target,			// TARGET
				0,			// LEVEL
				0,			// X OFFSET
				0,			// Y OFFSET
				tex_data[i].w,		// WIDTH
				tex_data[i].h,		// HEIGHT
				internal_format,	// FORMAT
				GL_UNSIGNED_BYTE,	// TYPE
				data			// DATA
		);

		return 0;
	}


	return -1;
}

#if(TARGET_ANDROID)
	typedef AAsset 			AssetType;
	typedef AAssetManager		AssetManagerType;

	static AssetType * _OpenAsset( AssetManagerType * manager, const char * file) {

		return AAssetManager_open( manager, file, AASSET_MODE_STREAMING);
	}

	static int _ReadAsset(AssetType * asset, void * ptr, size_t count) {

		return AAsset_read(asset, ptr, count);
	}

	static void _CloseAsset(AssetType * asset) {

		AAsset_close(asset);
	}
#else
	typedef FILE	AssetType;
	typedef void	AssetManagerType;

	static AssetType * _OpenAsset( AssetManagerType * manager, const char * file) {

		return fopen(file, "rb");
	}

	static int _ReadAsset(AssetType * asset, void * ptr, size_t count) {

		size_t r = fread(ptr, count, 1, asset);

		if(r == 1)
			return count;

		return 0;
	}

	static void _CloseAsset(AssetType * asset) {

		fclose(asset);
	}
#endif

int rh_texpak_open (/*AssetManagerType * asset_manager,*/ const char * gfx_file, rh_texpak_handle * loader_out) {
	
	AssetManagerType * asset_manager = NULL; // HACKED OUT ANDROID ASSET MANAGER!!!

	struct _texpak_type * loader = NULL;
	unsigned int uncompressed_buffer_size = 0;
	void * uncompressed_buffer = 0;
	unsigned int compressed_buffer_size = 0;
	void * compressed_buffer = NULL;
	struct rhtpak_hdr header;
	struct rhtpak_hdr_tex_data *tex_data = NULL;
	AssetType * asset = NULL;
	GLenum compressed_tex_format = -1;
	int i;

	GL_ERROR();

	if(!(loader = (struct _texpak_type *)calloc(1, sizeof(struct _texpak_type) )))
		goto err;

	if(!(asset = _OpenAsset( asset_manager, gfx_file )))
		goto err;

	if( _ReadAsset(asset, &header, sizeof header) != sizeof header )
		goto err;

	if(!(tex_data = (struct rhtpak_hdr_tex_data *)malloc(header.depth * sizeof(struct rhtpak_hdr_tex_data))))
		goto err;

	if( _ReadAsset(asset, tex_data, header.depth * sizeof(struct rhtpak_hdr_tex_data)) != header.depth * sizeof(struct rhtpak_hdr_tex_data))
		goto err;

	loader->hash_length = header.resources;
	loader->seed = header.seed;
	if(!(loader->hash = (struct rhtpak_hdr_hash *)malloc(loader->hash_length * sizeof(struct rhtpak_hdr_hash))))
		goto err;

	if( _ReadAsset(asset, loader->hash, header.resources * sizeof(struct rhtpak_hdr_hash)) != header.resources * sizeof(struct rhtpak_hdr_hash))
		goto err;

	compressed_tex_format = get_gl_compression_enum( header.format );

	loader->target = GL_TEXTURE_2D;
	
	if((header.depth > 1) && IsExtensionSupported("GL_EXT_texture_array"))
		loader->target = GL_TEXTURE_2D_ARRAY_EXT;

	if(loader->target == GL_TEXTURE_2D_ARRAY_EXT)
		loader->textures_length = 1;
	else
		loader->textures_length = header.depth;

	loader->textures = (GLuint*)calloc(1, sizeof(GLuint) * loader->textures_length );
	glGenTextures(loader->textures_length, loader->textures);
	glActiveTexture(GL_TEXTURE0);
	{
		int i;
		for(i=0;i<loader->textures_length;i++) {
			glBindTexture( loader->target, loader->textures[i]);
			glTexParameteri( loader->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri( loader->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri( loader->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri( loader->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if(loader->target == GL_TEXTURE_2D_ARRAY_EXT)
				allocate_texture_array_memory( &header, loader->target );
			else
				allocate_texture_memory( &header, loader->target );
		}
	}

	for(i=0;i<header.depth;i++) {

		if( tex_data[i].channel[0].file_length > compressed_buffer_size) {

			compressed_buffer_size = 0;
			free( compressed_buffer );
			if((compressed_buffer = malloc(tex_data[i].channel[0].file_length)))
				compressed_buffer_size = tex_data[i].channel[0].file_length;
		}

		if( tex_data[i].channel[0].uncompressed_size > uncompressed_buffer_size) {

			uncompressed_buffer_size = 0;
			free( uncompressed_buffer );
			if((uncompressed_buffer = malloc(tex_data[i].channel[0].uncompressed_size)))
				uncompressed_buffer_size = tex_data[i].channel[0].uncompressed_size;
		}

		if(uncompressed_buffer && compressed_buffer) {

			if( _ReadAsset(asset, compressed_buffer, compressed_buffer_size) != compressed_buffer_size)
				goto err;

			if( LZ4_uncompress( (const char *)compressed_buffer, (char*)uncompressed_buffer, uncompressed_buffer_size ) < 0 )
				goto err;

			if(loader->target == GL_TEXTURE_2D_ARRAY_EXT) {
				if(load_texture_array_data( tex_data, i, uncompressed_buffer, uncompressed_buffer_size, loader->target  ) != 0)
					goto err;
			}
			else {

				glBindTexture( loader->target, loader->textures[i]);

				if(load_texture_data( tex_data, i, uncompressed_buffer, uncompressed_buffer_size, loader->target  ) != 0)
					goto err;
			}
		}
		else
			goto err;
	}

	free(tex_data);
	free(uncompressed_buffer);
	free(compressed_buffer);
	if(asset) _CloseAsset(asset);

	if(GL_ERROR() == 0) {

		*loader_out = loader;

		return 0;
	}

err:

	GL_ERROR();

	if(loader) {
		if(loader->textures)
			glDeleteTextures(loader->textures_length, loader->textures);
		free(loader->textures);
		free(loader->hash);
		free(loader);
	}

	free(tex_data);
	free(uncompressed_buffer);
	free(compressed_buffer);
	if(asset) _CloseAsset(asset);

	return -1;
}

int rh_texpak_close(rh_texpak_handle loader) {

	if(loader) {

		free(loader->hash);
		if(loader->textures)
			glDeleteTextures(loader->textures_length, loader->textures);
		free(loader->textures);
		free(loader);
	}

	return 0;
}

static unsigned int hash( const char* _s, unsigned int seed)
{
    const char * s = _s;
    unsigned int hash = seed;
    int len = strlen(s);
    int i;

    while(len) {
    	len--;
    	if(s[len]=='.')
    		break;
    }

    for(i=0;i<len;i++)
    {
    	char c = *s++;

    	switch(c) {
    	case '/':
    	case '\\':
    		c = '.';
    		break;
    	default:
    		c = tolower(c);
    		break;
    	}

    	// THANKS PAUL LARSON.
        hash = hash * 101 + c;
    }

    return hash;
}

static int compare_hash(const void * key, const void * memb) {

	const unsigned int    * k = (const unsigned int *)key;
	const struct rhtpak_hdr_hash * m = (const struct rhtpak_hdr_hash*)memb;

	if( *k < m->hash )
		return -1;
	if( *k > m->hash )
		return 1;
	return 0;
}

int rh_texpak_lookup(rh_texpak_handle loader, const char * name, rh_texpak_idx * idx) {
  
  if(loader && idx && name) {

    unsigned int key = hash( name, loader->seed );

    struct rhtpak_hdr_hash * res = (struct rhtpak_hdr_hash *)bsearch(&key, loader->hash, loader->hash_length, sizeof(struct rhtpak_hdr_hash ), &compare_hash);

    if(res) {

      rh_texpak_idx i = (rh_texpak_idx)(res);
      rh_texpak_idx b = (rh_texpak_idx)(loader->hash);
      
      *idx = (i-b)/sizeof(struct rhtpak_hdr_hash );

      return 0;
    }
  }

  return -1;
}

int rh_texpak_get_texture(rh_texpak_handle loader, rh_texpak_idx idx, GLuint *tex) {
 
  *tex = loader->textures[ loader->hash[idx].i ];
  
  return 0;
}

int rh_texpak_get_size(rh_texpak_handle loader, rh_texpak_idx idx, unsigned int *w, unsigned int *h) {
 
  if(w) *w = loader->hash[idx].w;
  if(h) *h = loader->hash[idx].h;
  
  return 0;
}

int rh_texpak_get_depthi(rh_texpak_handle loader, rh_texpak_idx idx, unsigned int *i) {
 
  *i = loader->hash[idx].i;
}

int rh_texpak_get_depthf(rh_texpak_handle loader, rh_texpak_idx idx, GLfloat *f) {
  
  *f = loader->hash[idx].tex_coords[0].p;
}

int rh_texpak_get_coords2d(rh_texpak_handle loader, rh_texpak_idx idx, GLfloat *coords) {
   
  coords[0] = loader->hash[idx].tex_coords[0].s;
  coords[1] = loader->hash[idx].tex_coords[0].t;
  coords[2] = loader->hash[idx].tex_coords[1].s;
  coords[3] = loader->hash[idx].tex_coords[1].t;
  coords[4] = loader->hash[idx].tex_coords[2].s;
  coords[5] = loader->hash[idx].tex_coords[2].t;
  coords[6] = loader->hash[idx].tex_coords[3].s;
  coords[7] = loader->hash[idx].tex_coords[3].t;
}

int rh_texpak_get_coords3d(rh_texpak_handle loader, rh_texpak_idx idx, GLfloat *coords) {
  
  memcpy( coords, loader->hash[idx].tex_coords, sizeof loader->hash[idx].tex_coords ); 
  return 0;
}

int rh_texpak_get_textures(rh_texpak_handle loader, int *texcount) {

  *texcount = loader->textures_length;
  return 0;
}

int rh_texpak_get_textarget(rh_texpak_handle loader, GLenum *target) {

  
  return 0;
}

