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
  comservatory 
  GIT_REPOSITORY https://github.com/ArtifactDB/comservatory
  GIT_TAG master
)

FetchContent_MakeAvailable(uzuki2)
FetchContent_MakeAvailable(ritsuko)
FetchContent_MakeAvailable(comservatory)
