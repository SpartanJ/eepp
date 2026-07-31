#include "consolecommands.hpp"
#include "app.hpp"

static std::string createSavePath( const std::string& oriPath, Uint32 width, Uint32 height,
								   Image::SaveType saveType ) {
	Image::SaveType type = saveType == Image::SaveType::Unknown
							   ? Image::extensionToSaveType( FileSystem::fileExtension( oriPath ) )
							   : saveType;

	if ( Image::SaveType::Unknown == type ) {
		type = Image::SaveType::PNG;
	}

	return FileSystem::fileRemoveExtension( oriPath ) + "-" + String::toString( width ) + "x" +
		   String::toString( height ) + "." + Image::saveTypeToExtension( type );
}

Image::SaveType ConsoleCommands::getPathSaveType( const std::string& path ) {
	return Image::extensionToSaveType( FileSystem::fileExtension( path ) );
}

void ConsoleCommands::scaleImg( const std::string& Path, const Float& Scale,
								const bool& overridePath, Image::SaveType saveType ) {
	int w, h, c;

	if ( Image::getInfo( Path, &w, &h, &c ) && Scale > 0.f ) {
		Int32 new_width = static_cast<Int32>( w * Scale );
		Int32 new_height = static_cast<Int32>( h * Scale );
		std::string outputPath( Path );

		if ( !overridePath ) {
			outputPath = createSavePath( Path, new_width, new_height, saveType );
		}

		resizeImg( Path, outputPath, new_width, new_height, saveType );
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void ConsoleCommands::resizeImg( const std::string& Path, const std::string& outputPath,
								 const Uint32& NewWidth, const Uint32& NewHeight,
								 Image::SaveType saveType ) {
	if ( App::isImage( Path ) ) {
		Image::SaveType type =
			Image::SaveType::Unknown != saveType ? saveType : getPathSaveType( outputPath );

		Image img( Path );

		img.resize( NewWidth, NewHeight );

		img.saveToFile( outputPath, type );
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void ConsoleCommands::thumbnailImg( const std::string& Path, const Uint32& MaxWidth,
									const Uint32& MaxHeight, Image::SaveType saveType ) {
	if ( App::isImage( Path ) ) {
		Image img( Path );

		Image* thumb = img.thumbnail( MaxWidth, MaxHeight );

		if ( NULL != thumb ) {
			std::string newPath(
				createSavePath( Path, thumb->getWidth(), thumb->getHeight(), saveType ) );
			Image::SaveType type =
				Image::SaveType::Unknown != saveType ? saveType : getPathSaveType( newPath );

			thumb->saveToFile( newPath, type );

			eeSAFE_DELETE( thumb );
		}
	} else {
		Con->pushText( "Images does not exists." );
	}
}

void ConsoleCommands::centerCropImg( const std::string& Path, const Uint32& Width,
									 const Uint32& Height, Image::SaveType saveType ) {
	if ( App::isImage( Path ) ) {
		Image img( Path );

		Sizei nSize;

		double scale = 1.f;

		scale = eemax( (double)Width / (double)img.getWidth(),
					   (double)Height / (double)img.getHeight() );

		nSize.x = Math::round( img.getWidth() * scale );
		nSize.y = Math::round( img.getHeight() * scale );

		if ( nSize.getWidth() == (int)Width - 1 || nSize.getWidth() == (int)Width + 1 ) {
			nSize.x = (int)Width;
		}

		if ( nSize.getHeight() == (int)Height - 1 || nSize.getHeight() == (int)Height + 1 ) {
			nSize.y = (int)Height;
		}

		img.resize( nSize.getWidth(), nSize.getHeight() );

		Image* croppedImg = NULL;
		Rect rect;

		if ( img.getWidth() > Width ) {
			rect.Left = ( img.getWidth() - Width ) / 2;
			rect.Right = rect.Left + Width;
			rect.Top = 0;
			rect.Bottom = Height;
		} else {
			rect.Top = ( img.getHeight() - Height ) / 2;
			rect.Bottom = rect.Top + Height;
			rect.Left = 0;
			rect.Right = Width;
		}

		croppedImg = img.crop( rect );

		if ( NULL != croppedImg ) {
			std::string newPath(
				createSavePath( Path, croppedImg->getWidth(), croppedImg->getHeight(), saveType ) );
			Image::SaveType type =
				Image::SaveType::Unknown != saveType ? saveType : getPathSaveType( newPath );

			croppedImg->saveToFile( newPath, type );

			eeSAFE_DELETE( croppedImg );
		} else {
			std::string newPath(
				createSavePath( Path, img.getWidth(), img.getHeight(), saveType ) );
			Image::SaveType type =
				Image::SaveType::Unknown != saveType ? saveType : getPathSaveType( newPath );

			img.saveToFile( newPath, type );
		}
	}
}

void ConsoleCommands::batchImgScale( const std::string& Path, const Float& Scale,
									 const bool& overridePath ) {
	std::string iPath = Path;
	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( iPath );

	if ( iPath[iPath.size() - 1] != '/' )
		iPath += "/";

	for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
		std::string fPath = iPath + tmpFiles[i];

		scaleImg( fPath, Scale, overridePath );
	}
}

void ConsoleCommands::batchImgThumbnail( Sizei size, std::string dir, bool recursive ) {
	FileSystem::dirAddSlashAtEnd( dir );

	std::vector<std::string> files = FileSystem::filesGetInPath( dir );

	for ( size_t i = 0; i < files.size(); i++ ) {
		std::string fpath( dir + files[i] );

		if ( FileSystem::isDirectory( fpath ) ) {
			if ( recursive ) {
				batchImgThumbnail( size, fpath, recursive );
			}
		} else {
			int w, h, c;
			if ( Image::getInfo( fpath, &w, &h, &c ) ) {
				if ( w > size.getWidth() || h > size.getHeight() ) {
					Image img( fpath );

					Image* thumb = img.thumbnail( size.getWidth(), size.getHeight() );

					if ( NULL != thumb ) {
						thumb->saveToFile( fpath, Image::extensionToSaveType(
													  FileSystem::fileExtension( fpath ) ) );

						Con->pushText(
							"Thumbnail created for '%s'. Old size %dx%d. New size %dx%d.",
							fpath.c_str(), img.getWidth(), img.getHeight(), thumb->getWidth(),
							thumb->getHeight() );

						eeSAFE_DELETE( thumb );
					} else {
						Con->pushText( "Thumbnail %s failed to create.", fpath.c_str() );
					}
				}
			}
		}
	}
}

void ConsoleCommands::cmdSlideShow( const std::vector<String>& params ) {
	String Error( "Usage example: slideshow slide_time_in_ms" );

	if ( params.size() >= 2 ) {
		Uint32 time = 0;

		bool Res = String::fromString( time, params[1] );

		if ( Res ) {
			if ( !mApp->getSlideShow() ) {
				mApp->createSlideShow( time );
			} else {
				if ( 0 == time ) {
					mApp->setSlideShow( false );
				}
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdImgResize( const std::vector<String>& params ) {
	String Error(
		"Usage example: imgresize new_width new_height path_to_img format override_image_path" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::Unknown;
		Uint32 override = 0;

		bool Res1 = String::fromString( nWidth, params[1] );
		bool Res2 = String::fromString( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}

			if ( params.size() > 5 ) {
				String::fromString( override, params[5] );
			}
		} else {
			myPath = mApp->getFilePath();
		}

		if ( Res1 && Res2 ) {
			std::string savePath =
				override != 0 ? myPath : createSavePath( myPath, nWidth, nHeight, saveType );

			resizeImg( myPath, savePath, nWidth, nHeight, saveType );
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdImgThumbnail( const std::vector<String>& params ) {
	String Error( "Usage example: imgthumbnail max_width max_height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::Unknown;

		bool Res1 = String::fromString( nWidth, params[1] );
		bool Res2 = String::fromString( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}
		} else {
			myPath = mApp->getFilePath();
		}

		if ( Res1 && Res2 ) {
			thumbnailImg( myPath, nWidth, nHeight, saveType );
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdImgCenterCrop( const std::vector<String>& params ) {
	String Error( "Usage example: imgcentercrop width height path_to_img format" );
	if ( params.size() >= 3 ) {
		Uint32 nWidth = 0;
		Uint32 nHeight = 0;
		Image::SaveType saveType = Image::SaveType::Unknown;

		bool Res1 = String::fromString( nWidth, params[1] );
		bool Res2 = String::fromString( nHeight, params[2] );

		std::string myPath;

		if ( params.size() >= 4 ) {
			myPath = params[3].toUtf8();

			if ( params.size() > 4 ) {
				saveType = Image::extensionToSaveType( params[4] );
			}
		} else {
			myPath = mApp->getFilePath();
		}

		if ( Res1 && Res2 )
			centerCropImg( myPath, nWidth, nHeight, saveType );
		else
			Con->pushText( Error );
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdImgScale( const std::vector<String>& params ) {
	String Error( "Usage example: imgscale scale path_to_img format override_path" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		Image::SaveType saveType = Image::SaveType::Unknown;
		Uint32 override = 0;

		bool Res = String::fromString( Scale, params[1] );

		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].toUtf8();

			if ( params.size() > 3 ) {
				saveType = Image::extensionToSaveType( params[3] );
			}

			if ( params.size() > 4 ) {
				String::fromString( override, params[4] );
			}
		} else {
			myPath = mApp->getFilePath();
		}

		if ( Res )
			scaleImg( myPath, Scale, 0 != override, saveType );
		else
			Con->pushText( Error );
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdBatchImgScale( const std::vector<String>& params ) {
	String Error(
		"Usage example: batchimgscale scale_value override_img_path ( default disabled ) "
		"directory_to_resize_img ( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 2 ) {
		Float Scale = 0;
		Uint32 override = 0;

		bool Res = String::fromString( Scale, params[1] );

		override = String::fromString( override, params[2] );

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mApp->getFileDirectoryPath();

		if ( Res ) {
			if ( FileSystem::isDirectory( myPath ) ) {
				batchImgScale( myPath, Scale, 0 != override );
			} else {
				Con->pushText( "Second argument is not a directory!" );
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdBatchImgThumbnail( const std::vector<String>& params ) {
	String Error(
		"Usage example: batchimgthumbnail max_width max_height directory_to_create_thumbs "
		"recursive ( if no dir is passed, it will use the current dir opened )" );

	if ( params.size() >= 3 ) {
		Uint32 max_width = 0, max_height = 0;
		bool recursive = false;

		bool Res1 = String::fromString( max_width, params[1] );
		bool Res2 = String::fromString( max_height, params[2] );

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mApp->getFileDirectoryPath();

		if ( params.size() > 4 && params[4] == "recursive" ) {
			recursive = true;
		}

		if ( Res1 && Res2 ) {
			if ( FileSystem::isDirectory( myPath ) ) {
				batchImgThumbnail( Sizei( max_width, max_height ), myPath, recursive );
			} else {
				Con->pushText( "Third argument is not a directory!" );
			}
		} else {
			Con->pushText( Error );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdImgChangeFormat( const std::vector<String>& params ) {
	String Error( "Usage example: imgchangeformat to_format image_to_reformat ( if null will use "
				  "the current loaded image )" );
	if ( params.size() >= 2 ) {
		std::string toFormat = params[1].toUtf8();
		std::string myPath;

		if ( params.size() >= 3 ) {
			myPath = params[2].toUtf8();
		} else {
			myPath = mApp->getFilePath();
		}

		std::string fromFormat = FileSystem::fileExtension( myPath );

		if ( Image::isImage( myPath ) ) {
			std::string fPath = myPath;
			std::string fExt = FileSystem::fileExtension( fPath );

			if ( fExt == fromFormat ) {
				std::string fName;

				if ( fExt != toFormat )
					fName = fPath.substr( 0, fPath.find_last_of( "." ) + 1 ) + toFormat;
				else
					fName = fPath + "." + toFormat;

				Image::SaveType saveType = Image::extensionToSaveType( toFormat );

				if ( Image::SaveType::Unknown != saveType ) {
					Image* img = eeNew( Image, ( fPath ) );
					img->saveToFile( fName, saveType );
					eeSAFE_DELETE( img );

					Con->pushText( fName + " created." );
				}
			}
		} else {
			Con->pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdBatchImgChangeFormat( const std::vector<String>& params ) {
	String Error( "Usage example: batchimgchangeformat from_format to_format directory_to_reformat "
				  "( if no dir is passed, it will use the current dir opened )" );
	if ( params.size() >= 4 ) {
		std::string fromFormat = params[1].toUtf8();
		std::string toFormat = params[2].toUtf8();

		std::string myPath = params.size() >= 4 ? params[3].toUtf8() : mApp->getFileDirectoryPath();

		if ( FileSystem::isDirectory( myPath ) ) {
			std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( myPath );

			if ( myPath[myPath.size() - 1] != '/' )
				myPath += "/";

			for ( Int32 i = 0; i < (Int32)tmpFiles.size(); i++ ) {
				std::string fPath = myPath + tmpFiles[i];
				std::string fExt = FileSystem::fileExtension( fPath );

				if ( App::isImage( fPath ) && fExt == fromFormat ) {
					std::string fName;

					if ( fExt != toFormat )
						fName = fPath.substr( 0, fPath.find_last_of( "." ) + 1 ) + toFormat;
					else
						fName = fPath + "." + toFormat;

					Image::SaveType saveType = Image::extensionToSaveType( toFormat );

					if ( Image::SaveType::Unknown != saveType ) {
						Image* img = eeNew( Image, ( fPath ) );
						img->saveToFile( fPath, saveType );
						eeSAFE_DELETE( img );

						Con->pushText( fName + " created." );
					}
				}
			}
		} else {
			Con->pushText( "Third argument is not a directory! Argument: " + myPath );
		}
	} else {
		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdMoveTo( const std::vector<String>& params ) {
	if ( params.size() >= 2 && mApp->getFiles().size() > 0 ) {
		Int32 tInt = 0;

		bool Res = String::fromString( tInt, params[1] );

		if ( tInt )
			tInt--;

		if ( Res && tInt >= 0 && tInt < (Int32)mApp->getFiles().size() ) {
			Con->pushText( "moveto: moving to image number " + String::toString( tInt + 1 ) );
			mApp->fastLoadImage( tInt );
		} else if ( params[1] == "last" ) {
			Con->pushText( "moveto: moving to last" );
			mApp->fastLoadImage( mApp->getFiles().size() - 1 );
		} else if ( params[1] == "first" ) {
			Con->pushText( "moveto: moving to first" );
			mApp->fastLoadImage( 0 );
		} else {
			Con->pushText( "moveto: image number does not exists" );
		}
	} else {
		Con->pushText( "Expected some parameter" );
	}
}

void ConsoleCommands::cmdSetBlockWheel( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mApp->getConfig().BlockWheelSpeed = tInt ? true : false;
			Con->pushText( "setblockwheel " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void ConsoleCommands::cmdSetLateLoading( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mApp->getConfig().LateLoading = tInt ? true : false;
			Con->pushText( "setlateloading " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void ConsoleCommands::cmdSetImgFade( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::fromString( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mApp->getConfig().Fade = tInt ? true : false;
			Con->pushText( "setimgfade " + String::toString( tInt ) );
		} else
			Con->pushText( "Valid parameters are 0 or 1." );
	} else
		Con->pushText( "Expected some parameter" );
}

void ConsoleCommands::cmdSetBackColor( const std::vector<String>& params ) {
	String Error(
		"Usage example: setbackcolor 255 255 255 (RGB Color, numbers between 0 and 255)" );

	if ( params.size() >= 2 ) {
		if ( params.size() >= 2 ) {
			mApp->getWindow()->setClearColor( Color::fromString( params[1].toUtf8() ).toRGB() );
			Con->pushText( "setbackcolor applied" );
			return;
		}

		Con->pushText( Error );
	}
}

void ConsoleCommands::cmdLoadImg( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();

		if ( App::isImage( myPath ) || App::isHttpUrl( myPath ) ) {
			mApp->loadDir( myPath );
		} else
			Con->pushText( "\"" + myPath +
						   "\" is not an image path or the image is not supported." );
	}
}

void ConsoleCommands::cmdLoadDir( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		std::string myPath = params[1].toUtf8();
		if ( params.size() > 2 ) {
			for ( Uint32 i = 2; i < params.size(); i++ )
				myPath += " " + params[i].toUtf8();
		}

		if ( FileSystem::isDirectory( myPath ) ) {
			mApp->loadDir( myPath );
		} else
			Con->pushText( "If you want to load an image use loadimg. \"" + myPath +
						   "\" is not a directory path." );
	}
}

void ConsoleCommands::cmdSetZoom( const std::vector<String>& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0;

		bool Res = String::fromString( tFloat, params[1] );

		if ( Res && tFloat >= 0 && tFloat <= 10 ) {
			Con->pushText( "setzoom: zoom level " + String::toString( tFloat ) );
			mApp->setImgScale( tFloat );
		} else
			Con->pushText( "setzoom: value out of range" );
	} else
		Con->pushText( "Expected some parameter" );
}
