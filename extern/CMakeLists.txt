include(FetchContent)

FetchContent_Declare(
  uzuki2
  GIT_REPOSITORY https://github.com/ArtifactDB/uzuki2
  GIT_TAG master
)

FetchContent_Declare(
  ritsuko 
  GIT_REPOSITORY https://github.com/ArtifactDB/ritsuko
  GIT_TAG master
)

FetchContent_Declare(
  chihaya
  GIT_REPOSITORY https://github.com/ArtifactDB/chihaya
  GIT_TAG master
)

FetchContent_MakeAvailable(uzuki2)
FetchContent_MakeAvailable(ritsuko)
FetchContent_MakeAvailable(chihaya)
