
set(Utils_ExternalTarget TRUE)
set(Utils_TargetFolders TRUE)
set(Utils_TargetName  Utils)
set(Utils_TargetGroup Extern)
subdirs(Utils)

if (InspectionTools_BUILD_SDL)
    subdirs(SDL)
    set(Math_ExternalTarget TRUE)
    set(Math_TargetFolders TRUE)
    set(Math_TargetName  Math)
    set(Math_TargetGroup Extern)
    subdirs(Math)

    set(FreeType_ExternalTarget TRUE)
    set(FreeType_TargetFolders TRUE)
    set(FreeType_TargetName    Freetype)
    set(FreeType_TargetGroup   Extern)

    subdirs(Freetype)

    set(FreeImage_ExternalTarget TRUE)
    set(FreeImage_TargetFolders  TRUE)
    set(FreeImage_TargetName     FreeImage)
    set(FreeImage_TargetGroup    Extern)

    subdirs(FreeImage)


    set(Image_ExternalTarget TRUE)
    set(Image_TargetFolders  TRUE)
    set(Image_TargetName     Image)
    set(Image_TargetGroup    Extern)

    subdirs(Image)
endif()

