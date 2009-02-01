from BeautifulSoup import BeautifulSoup, BeautifulStoneSoup, Tag
import re
import codecs
import sys

sets ={'BE':{'name':'Beta',
             'dir':'BE',
             'abbrev':'BE',
             'gathabbrev': '2E',
             'gathname':'LimitedEditionBeta',
             },
       'RV':{'name':'Revised',
             'dir':'RV',
             'abbrev':'RV',
             'gathdirs':['3E/en-us'],
             'gathabbrev': '3E',
             'gathname':'RevisedEdition'
             },
       '4E':{'name':'4th Edition',
             'dir':'4E',
             'abbrev':'4E',
             'gathdirs':['4E/en-us'],
             'gathabbrev': '4E',
             'gathname':'FourthEdition',
             },
       '5E':{'name':'5th Edition',
             'dir':'5E',
             'abbrev':'5E',
             'gathdirs':['5E/en-us'],
             'gathabbrev': '5E',
             'gathname':'FifthEdition'
             },
       '6E':{'name':'6th Edition',
             'dir':'6E',
             'abbrev':'6E',
             'gathdirs':['6E/en-us'],
             'gathabbrev': '6E',
             'gathname':'ClassicSixthEdition',
             },
       '7E':{'name':'7th Edition',
             'dir':'7E',
             'abbrev':'7E',
             'gathdirs':['7E/en-us'],
             'gathabbrev': '7E',
             'gathname':'SeventhEdition',
             },
       '8E':{'name':'8th Edition',
             'dir':'8E',
             'abbrev':'8E',
             'gathdirs':['8ED/en-us'],
             'gathabbrev': '8ED',
             'gathname':'EighthEdition',
             },
       '9E':{'name':'9th Edition',
             'dir':'9E',
             'abbrev':'9E',
             'gathdirs':['9ED/en-us'],
             'gathabbrev': '9ED',
             'gathname':'NinthEdition',
             },
       '10E':{'name':'10th Edition',
             'dir':'10E',
             'abbrev':'10E',
             'gathdirs':['10E/EN'],
             'gathabbrev': '10E',
             'gathname':'TenthEdition',
             },
       'EVE':{'name':'Eventide',
             'dir':'EVE',
             'abbrev':'EVE',
             'gathdirs':['EVE/EN'],
             'gathabbrev': 'EVE',
             'gathname':'Eventide',
             },
       'SHM':{'name':'Shadowmoor',
             'dir':'SHM',
             'abbrev':'SHM',
             'gathdirs':['SHM/EN'],
             'gathabbrev': 'SHM',
             'gathname':'Shadowmoor',
             },       
        'ALA':{'name':'Shards of Alara',
             'dir':'ALA',
             'abbrev':'ALA',
             'gathdirs':['ALA/EN'],
             'gathabbrev': 'ALA',
             'gathname':'ShardsOfAlara',
             },
        'CFX':{'name':'Conflux',
             'dir':'CFX',
             'abbrev':'CFX',
             'gathdirs':['CFX/EN'],
             'gathabbrev': 'CFX',
             'gathname':'Conflux'
             },      
       'UH':{'name':'Unhinged',
             'dir':'UH',
             'abbrev':'UH',
             'gathdirs':['UNH/en-us'],
             'gathabbrev': 'UNH',
             'gathname':'Unhinged',
             },
       'UG':{'name':'Unglued',
             'dir':'UG',
             'abbrev':'UG',
             'gathdirs':['UG/en-us'],
             'gathabbrev':'UG',
             'gathname':'Unglued',
             },
       'P1':{'name':'Portal',
             'dir':'PO',
             'abbrev':'PO',
             'gathdirs':['PO/en-us'],
             'gathabbrev':'PO',
             'gathname':'Portal',
             },
       'P2':{'name':'Portal Second Age',
             'dir':'P2',
             'abbrev':'P2',
             'gathdirs':['P2/en-us'],
             'gathabbrev':'P2',
             'gathname':'PortalSecondAge',
             },
       'P3':{'name':'Portal Three Kingdoms',
             'dir':'P3',
             'abbrev':'P3',
             'gathdirs':['PK/en-us'],
             'gathabbrev': 'PK',
             'gathname':'PortalThreeKingdoms',
             },
       'AN':{'name':'Arabian Nights',
             'dir':'AN',
             'abbrev':'AN',
             'gathdirs':['AN/en-us'],
             'gathabbrev': 'AN',
             'gathname':'ArabianNights'
             },
       'AQ':{'name':'Antiquities',
             'dir':'AQ',
             'abbrev':'AQ',
             'gathdirs':['AQ/en-us'],
             'gathabbrev': 'AQ',
             'gathname':'Antiquities',
             },
       'LG':{'name':'Legends',
             'dir':'LG',
             'abbrev':'LG',
             'gathdirs':['LG/en-us'],
             'gathabbrev': 'LE',
             'gathname':'Legends',
             },
       'DK':{'name':'The Dark',
             'dir':'DK',
             'abbrev':'DK',
             'gathdirs':['DK/en-us'],
             'gathabbrev': 'DK',
             'gathname':'TheDark',
             },
       'FE':{'name':'Fallen Empires',
             'dir':'FE',
             'abbrev':'FE',
             'gathdirs':['FE/en-us'],
             'gathabbrev': 'FE',
             'gathname':'FallenEmpires',
             },
       'IA':{'name':'Ice Age',
             'dir':'IA',
             'abbrev':'IA',
             'gathdirs':['IA/en-us'],
             'gathabbrev': 'IA',
             'gathname':'IceAge',
             },
       'HL':{'name':'Homelands',
             'dir':'HL',
             'abbrev':'HL',
             'gathdirs':['HM/en-us'],
             'gathabbrev': 'HM',
             'gathname':'Homelands'
             },
       'AL':{'name':'Alliances',
             'dir':'AL',
             'abbrev':'AL',
             'gathdirs':['AL/en-us'],
             'gathabbrev': 'AL',
             'gathname':'Alliances',
             },
       'MI':{'name':'Mirage',
             'dir':'MI',
             'abbrev':'MI',
             'gathdirs':['MI/en-us'],
             'gathabbrev': 'MI',
             'gathname':'Mirage',
             },
       'VI':{'name':'Visions',
             'dir':'VI',
             'abbrev':'VI',
             'gathabbrev': 'VI',
             'gathname':'Visions',
             },
       'WL':{'name':'Weatherlight',
             'dir':'WL',
             'abbrev':'WL',
             'gathabbrev': 'WL',
             'gathname':'Weatherlight',
             },
       'TE':{'name':'Tempest',
             'dir':'TE',
             'gathdirs':['TE/en-us'],
             'abbrev':'TE',
             'gathabbrev': 'TE',
             'gathname':'Tempest',
             },
       'SH':{'name':'Stronghold',
             'dir':'SH',
             'abbrev':'SH',
             'gathabbrev': 'ST',
             'gathname':'Stronghold',
             },
       'EX':{'name':'Exodus',
             'dir':'EX',
             'abbrev':'EX',
             'gathabbrev': 'EX',
             'gathname':'Exodus',
             },
       'US':{'name':'Urza\'s Saga',
             'dir':'US',
             'abbrev':'US',
             'gathabbrev': 'UZ',
             'gathname':'UrzasSaga',
             },
       'UL':{'name':'Urza\'s Legacy',
             'dir':'UL',
             'abbrev':'UL',
             'gathabbrev': 'GU',
             'gathname':'UrzasDestiny',
             },
       'UD':{'name':'Urza\'s Destiny',
             'dir':'UD',
             'abbrev':'UD',
             'gathabbrev': 'CG',
             'gathname':'UrzasLegacy',
             },
       'MM':{'name':'Mercadian Masques',
             'dir':'MM',
             'abbrev':'MM',
             'gathabbrev': 'MM',
             'gathname':'MercadianMasques',
             },
       'NE':{'name':'Nemesis',
             'dir':'NE',
             'abbrev':'NE',
             'gathabbrev': 'NE',
             'gathname':'Nemesis',
             },
       'PY':{'name':'Prophecy',
             'dir':'PY',
             'abbrev':'PY',
             'gathabbrev': 'PR',
             'gathname':'Prophecy',
             },
       'IN':{'name':'Invasion',
             'dir':'IN',
             'abbrev':'IN',
             'gathabbrev': 'IN',
             'gathname':'Invasion',
             },
       'PS':{'name':'Planeshift',
             'dir':'PS',
             'abbrev':'PS',
             'gathabbrev': 'PS',
             'gathname':'Planeshift',
             },
       'AP':{'name':'Apocalypse',
             'dir':'AP',
             'abbrev':'AP',
             'gathabbrev': 'AP',
             'gathname':'Apocalypse',
             },
       'OD':{'name':'Odyssey',
             'dir':'OD',
             'abbrev':'OD',
             'gathabbrev': 'OD',
             'gathname':'Odyssey',
             },
       'TO':{'name':'Torment',
             'dir':'TO',
             'abbrev':'TO',
             'gathabbrev': 'TOR',
             'gathname':'Torment',
             },
       'JD':{'name':'Judgment',
             'dir':'JD',
             'abbrev':'JD',
             'gathabbrev': 'JUD',
             'gathname':'Judgment',
             },
       'ON':{'name':'Onslaught',
             'dir':'ON',
             'abbrev':'ON',
             'gathabbrev': 'ONS',
             'gathname':'Onslaught',
             },
       'LE':{'name':'Legions',
             'dir':'LE',
             'abbrev':'LE',
             'gathabbrev': 'LGN',
             'gathname':'Legions',
             },
       'SC':{'name':'Scourge',
             'dir':'SC',
             'abbrev':'SC',
             'gathabbrev': 'SCG',
             'gathname':'Scourge',
             },
       'MR':{'name':'Mirrodin',
             'dir':'MR',
             'abbrev':'MR',
             'gathabbrev': 'MRD',
             'gathname':'Mirrodin',
             },
       'DS':{'name':'Darksteel',
             'dir':'DS',
             'abbrev':'DS',
             'gathabbrev': 'DST',
             'gathname':'Darksteel',
             },
       'FD':{'name':'Fifth Dawn',
             'dir':'FD',
             'abbrev':'FD',
             'gathabbrev': '5DN',
             'gathname':'FifthDawn',
             },
       'CK':{'name':'Champions of Kamigawa',
             'dir':'CK',
             'abbrev':'CK',
	 'gathdirs':['CHK/en-us'],
             'gathabbrev': 'CHK',
             'gathname':'ChampionsofKamigawa',
             },
       'BK':{'name':'Betrayers of Kamigawa',
             'dir':'BK',
             'abbrev':'BK',
             'gathabbrev': 'BOK',
             'gathname':'BetrayersofKamigawa',
             },
       'SK':{'name':'Saviors of Kamigawa',
             'dir':'SK',
             'abbrev':'SK',
             'gathabbrev': 'SOK',
             'gathname':'SaviorsofKamigawa',
             },
       'RA':{'name':'Ravnica: City of Guilds',
             'dir':'RA',
             'abbrev':'RA',
             'gathabbrev': 'RAV',
             'gathname':'RavnicaCityofGuilds',
             },
       'GP':{'name':'Guildpact',
             'dir':'GP',
             'abbrev':'GP',
             'gathabbrev': 'GPT',                    
             'gathname':'Guildpact',
             },
       'DI':{'name':'Dissension',
             'dir':'DI',
             'abbrev':'DI',
             'gathabbrev': 'DIS',
             'gathname':'Dissension',
             },
       'CS':{'name':'Coldsnap',
             'dir':'CS',
             'abbrev':'CS',
             'gathabbrev':'CSP',
             'gathname':'Coldsnap',
             },
       'TS':{'name':'Time Spiral',
             'gathname':'TimeSpiralBlock',
             'gathabbrev':'(?:(?:TSP)|(?:TSB))',
             'dir':'TS',
             'abbrev':'TS',
             'gathdirs' : ('TSP','TSB'),
             },
       'PC':{'name':'Planar Chaos',
             'gathname':'Planar%20Chaos',
             'gathabbrev':'PLC',
             'dir':'PC',
             'abbrev':'PC',
             },
       'S1':{'name':'Starter 1999',
             'gathname':'Starter%201999',
             'gathabbrev':'P3',
             'gathdirs':['P3'],
             'dir':'S1',
             'abbrev':'S1'
             },
       'S2':{'name':'Starter 2000',
             'gathname':'Starter%202000',
             'gathabbrev':'P4',
             'dir':'S1',
             'abbrev':'S1'
             },
       'FS':{'name':'Future Sight',
             'gathname':'Future%20Sight',
             'gathabbrev':'FUT',
             'gathdirs':['FUT'],
             'dir':'FS',
             'abbrev':'FS'
             },
       }

def maketransU(s1, s2, todel=""):
    trans_tab = dict( zip( map(ord, s1), map(ord, s2) ) )
    trans_tab.update( (ord(c),None) for c in todel )
    return trans_tab

imagetrans = maketransU(u'\xe2\xea\xee\xf4\xfb\xe1\xe9\xed\xf3\xfa\xfd\xe4\xeb\xef\xf6\xfc\xff\xe5\xc2\xca\xce\xd4\xdb\xc1\xc9\xcd\xd3\xda\xdd\xc4\xcb\xcf\xd6\xdc\xc5',u'aeiouaeiouyaeiouyaAEIOUAEIOUYAEIOUA',u"'/,. &;!")
imagetrans[198]=u'AE'

nametrans = maketransU(u'\xe2\xea\xee\xf4\xfb\xe1\xe9\xed\xf3\xfa\xfd\xe4\xeb\xef\xf6\xfc\xff\xe5\xc2\xca\xce\xd4\xdb\xc1\xc9\xcd\xd3\xda\xdd\xc4\xcb\xcf\xd6\xdc\xc5',u'aeiouaeiouyaeiouyaAEIOUAEIOUYAEIOUA')
nametrans[198]=u'AE'

cleanuptrans = {ord(u'\r'):u'    ',
                ord(u'"'):u'&quot;',
                ord(u'\u2018'):ord(u'\''),
                ord(u'&'):u'&amp;',
                }

colorSymbols = {'red':'R',
               'green' : 'G',
               'blue':'U',
               'black':'B',
               'white':'W'}
symbolColors = dict([reversed(a) for a in colorSymbols.items()])
basic_lands = ('Mountain','Forest','Island','Swamp','Plains')
color_re = re.compile(".*(?P<color>[Rr]ed|[Gg]reen|[Bb]lue|[Bb]lack|[Ww]hite).*")
mana_re = re.compile(".*Symbol_(?P<type>.*)_mana\.gif.*")
tap_re = re.compile(".*tap.gif.*")
basicLand_re = re.compile("\[(?P<mana>.)\]")
split_re = re.compile("(?P<t1>.*)     //     (?P<name2>.*)     (?P<mana2>\{.*\})  (?P<type2>.*)     (?P<t2>.*)")
id_re = re.compile(".*id=(?P<id>\d*).*")
reminder_re = re.compile('(\A[^\(]*)|((?<=\))[^\(]*)')

_stripReminderText = True;

def replaceSymbols(soup):
    for symbol in soup.findAll('img'):
        m = color_re.match(str(symbol['src']))
        if m:
            s = colorSymbols[m.group('color').lower()]
            symbol.replaceWith('{' + s + '}')
            
        m = mana_re.match(str(symbol))
        if m:
            if m.group('type') == "Snow":
                symbol.replaceWith('{S}')
            else:
                symbol.replaceWith('{' + m.group('type') + '}')

        m = tap_re.match(str(symbol))
        if m:
            symbol.replaceWith('{T}')

    return soup


def getCardTypes(soup):
    types = [t.strip()
             for t in soup('td')[2]('font')[0].string.split('-',1)]
    if (len(types) == 2):
        supertype = types[0]
        subtype = types[1]
    else:
        supertype = types[0]
        subtype = ''
    # replace entities, since gccg doesn't undertand them in attributes
    subtype = subtype.replace("&#226;", u"\342")

    return supertype, subtype


def cleanupHTMLText(htmlText, stripReminder = _stripReminderText):
    for i in htmlText.findAll('br'):
        i.replaceWith(' ')
    for i in htmlText.findAll('i'):
        i.replaceWith(''.join(i.contents))
    for i in htmlText.findAll('b'):
        i.replaceWith(''.join(i.contents))

    text = htmlText('font')[0].renderContents(None)
    # add text for Basic Land
    m = basicLand_re.match(text)
    if m:
        text = u"{T}: Add {" + m.group('mana') + u"} to your mana pool."
    if text == u"&nbsp;":
        text = u""

    text = text.translate(cleanuptrans)

    if stripReminder:
        text = ''.join([''.join(m) for m in reminder_re.findall(text)])

    return text

