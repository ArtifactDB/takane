include(FetchContent)

FetchContent_Declare(
  ritsuko 
  GIT_REPOSITORY https://github.com/ArtifactDB/ritsuko
  GIT_TAG master
)

FetchContent_MakeAvailable(ritsuko)

FetchContent_Declare(
  comservatory 
  GIT_REPOSITORY https://github.com/ArtifactDB/comservatory
  GIT_TAG master
)

FetchContent_MakeAvailable(comservatory)
