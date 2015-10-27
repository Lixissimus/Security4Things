library(lattice)

data = read.csv("data.csv", head=TRUE)
barchart(success~speed,
         data=data,
         groups=phone,
         col=c("lightgray","darkgray"),
         horiz=FALSE,
         main="Successful transmissions",
         xlim=c("7","15","31","63"),
         xlab="Period length in ms",
         ylab="Percent of successful transmissions"
)