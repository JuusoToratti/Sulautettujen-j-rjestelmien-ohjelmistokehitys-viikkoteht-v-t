*** Settings ***
Library    serial_keywords.py
Documentation     UART-testit time_parser-funktiolle

Suite Setup       Open Port    COM9    115200    2
Suite Teardown    Close Port

*** Test Cases ***
*** Test Cases ***
Yhteystesti
    [Documentation]    Tarkistaa että yhteys toimii ja portti vastaa
    Write Data    000120\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    80


Oikea Aika Merkkijono
    [Documentation]    Testaa toimivan aikamerkkijonon parsinta (000120 → 80 sekuntia)
    Write Data    000120\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    80


Virheellinen Sekunti
    [Documentation]    Testaa virheellinen aika (sekunnit > 59 → virhekoodi -6)
    Write Data    001067\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -6


Liian Lyhyt Merkkijono
    [Documentation]    Testaa että liian lyhyt aikamerkkijono hylätään (pituus < 6)
    Write Data    0123\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -1


Liian Pitkä Merkkijono
    [Documentation]    Testaa että liian pitkä aikamerkkijono hylätään (pituus > 6)
    Write Data    0001209\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -1


Nolla Aika
    [Documentation]    Testaa että nolla-aika hylätään (000000 → virhe, koska ajastin turha)
    Write Data    000000\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -2


Ei Numeroita
    [Documentation]    Testaa että merkkijono, jossa on kirjaimia, hylätään (isdigit-filtteri)
    Write Data    00A120\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -3


Virheellinen Minuutti
    [Documentation]    Testaa virheelliset minuutit (minuutit > 59 → virhekoodi -5)
    Write Data    006160\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -5


Virheellinen Tunti
    [Documentation]    Testaa virheelliset tunnit (tunnit > 23 → virhekoodi -4)
    Write Data    240000\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -4


Yhdistelmävirhe
    [Documentation]    Testaa moninkertainen virhe (sekä kirjaimia että liian pitkä)
    Write Data    00A1208\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -3


Oikeellinen Raja-Arvo 1
    [Documentation]    Testaa suurin mahdollinen validi aika (235959 → 86399 sekuntia)
    Write Data    235959\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    86399


Oikeellinen Raja-Arvo 2
    [Documentation]    Testaa pienin mahdollinen validi aika (000001 → 1 sekunti)
    Write Data    000001\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    1


    Oikea Sekvenssi
    [Documentation]    Testaa toimivan sekvenssin (vain R, Y, G sallittu)
    Write Data    RYGRYG\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    OK

Virheellinen Sekvenssi
    [Documentation]    Testaa virheellinen sekvenssi (sisältää kielletyn merkin)
    Write Data    RjG\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -7

Tyhjä Sekvenssi
    [Documentation]    Testaa että tyhjä sekvenssi hylätään
    Write Data    \n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -8

Liian Pitkä Sekvenssi
    [Documentation]    Testaa että liian pitkä sekvenssi hylätään (yli 20 merkkiä)
    Write Data    RYGRYGRYGRYGRYGRYGRYG\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    -9

    Oikea Sekvenssi
    [Documentation]    Testaa oikea sekvenssi (esim. RYG)
    Write Data    S,RYG\n
    ${response}=    Read Until
    Log To Console    Vastaanotettu: ${response}
    Should Contain    ${response}    0