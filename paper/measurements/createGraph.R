library(lattice)

data = read.csv("data.csv", head=TRUE)
scale = 1.8;
barchart(success~speed,
         data=data,
         groups=phone,
         col=c("lightgray","darkgray"),
         horiz=FALSE,
         main=list(label="Successful transmissions",cex=scale),
         xlim=c("7","15","31","63"),
         xlab=list(label="Period length in ms", cex=scale),
         ylab=list(label="Percent of successful transmissions", cex=scale),
         scales=list(cex=1.5)
)