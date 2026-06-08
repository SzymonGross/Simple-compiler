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
    zakoncz fun(3,3)
}