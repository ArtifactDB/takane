include(FetchContent)

FetchContent_Declare(
  uzuki2
  GIT_REPOSITORY https://github.com/ArtifactDB/uzuki2
  GIT_TAG master #^2.0.0
)

FetchContent_Declare(
  ritsuko 
  GIT_REPOSITORY https://github.com/ArtifactDB/ritsuko
  GIT_TAG master #^0.6.0
)

FetchContent_Declare(
  chihaya
  GIT_REPOSITORY https://github.com/ArtifactDB/chihaya
  GIT_TAG master #^1.1.0
)

FetchContent_MakeAvailable(uzuki2)
FetchContent_MakeAvailable(ritsuko)
FetchContent_MakeAvailable(chihaya)
