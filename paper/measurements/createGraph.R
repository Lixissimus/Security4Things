data = read.csv("data.csv", head=TRUE)
barchart(fails~speed,
         data=data,
         groups=phone,
         col=c("lightgray","black"),
         horiz=FALSE,
         main="Failing transmissions",
         xlim=c("7","15","31","63"),
         xlab="Period length in ms",
         ylab="Number of failed transmissions"
)