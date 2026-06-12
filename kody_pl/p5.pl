liczba fun (liczba m, liczba n)
{
    jezeli m == 0
    {
        zakoncz n + 1
    }
    albojezeli n == 0
    {
        zakoncz fun(m-1, 1)
    }
    albo
    {
        zakoncz fun(m-1, fun(m,n-1))
    }
}

poczontek
{
    liczba n, m

    wczytaj n, m

    liczba x <- fun(n,m)
    wypisz "%x\n"

    zakoncz 0
}