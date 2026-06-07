liczba[15] tab
tab[0] <- 1
tab[1] <- 1

liczba i <- 0

dopoki i < 5 {
    tab[i+2] <- tab[i] + tab[i+1]
    i <- i+1
}

zakoncz tab[6]
