\# Zadania z rekrutacji do Space Systems AGH 2026



Rozwiązania zadań firmware 2.1 i 2.2

Oba moduły zostały skonfigurowane pod płytkę NUCLEO-G474RE. Program z ledami używa wbudowanego na niej przycisku pod PC13.



\## Obsługa programowalnej diody RGB

Obsługa ledów działa za pomocą biblioteki (należy w niej oraz main.c ustawić liczbę ledów). Jest ona dostosowana do timera 80MHz z ARR = 99.



\## Biblioteka TLV

Biblioteka zawiera abstrakcyjne interfejsy pozwalające na przetwarzanie ramek TLV. Można jej używać w projektach niezależnie od hardware'u (nie zawiera odwołań do biblioteki HAL)

Plik msg\_handler.c implementuje przetwarzanie ramek z msg.h



Do tych implementacji można należałoby dodać jeszcze odpowiednią obsługę błędów.

