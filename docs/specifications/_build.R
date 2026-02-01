library(knitr)

listings <- list.files(pattern="\\.Rmd$")
default <- "1.0"
known.variants <- list(
    atomic_vector.Rmd="1.1",
    dense_array.Rmd="1.1",
    data_frame.Rmd="1.1",
    simple_list.Rmd="1.1",
    spatial_experiment.Rmd=c("1.1", "1.2", "1.3")
)

dest <- "compiled"
unlink(dest, recursive=TRUE)
dir.create(dest)

for (n in listings) {
    versions <- c(default, known.variants[[n]])
    odir <- file.path(dest, sub("\\.Rmd$", "", n))
    unlink(odir, recursive=TRUE)
    dir.create(odir)
    for (v in versions) {
        .version <- package_version(v)
        knitr::knit(n, file.path(odir, paste0(v, ".md")))
    }
}

file.copy("_general.md", dest)
