from selenium import webdriver 
from selenium.webdriver.common.by import By
import os
from datetime import datetime

import struct
from array import array

def is_file_empty(file_title):
    return os.stat(file_title).st_size == 0

def scrape_svenska_spel(tips_type: str):
    tips_type = tips_type.lower()
    assert(tips_type in ['europatipset', 'stryktipset', 'topptipset'])
    file_title = create_filename_with_date(tipstype=tips_type)

    
    url = f'https://spela.svenskaspel.se/{tips_type}'
    driver = webdriver.Firefox()
    driver.minimize_window()

    driver.get(url)
    #Accept cookies
    driver.implicitly_wait(1)
    try:
        cookies_button = driver.find_element(by=By.CSS_SELECTOR,value='#onetrust-reject-all-handler')
    except Exception:
        cookies_button = driver.find_element(by=By.CSS_SELECTOR,value='#onetrust-accept-btn-handler')
        
    cookies_button.click()
    driver.implicitly_wait(5)
    main_elem = driver.find_element(by=By.ID, value="tipsen").text
    driver.close()
    betting_info = main_elem.split("\n")
    write_to_file(info_list=betting_info, file_title=file_title)

def create_filename_with_date(tipstype:str):
    today_date = datetime.now().strftime("%Y%m%d")
    filename = f"{tipstype.lower()}_Betting_Info_{today_date}.txt"
    return filename

# Function to extract Spelomgång title
def extract_spelomgang_title(info_list):
    for item in info_list:
        if item.startswith('Spelomgång'):
            return item.split(' ', 1)[1]

# Function to write betting info to a file
def write_to_file(info_list, file_title):
    with open(file_title, 'w') as file:
        for item in info_list:
            file.write(f"{item}\n")



def read_betting_info(file_title):
    with open(file_title, 'r') as file:
        betting_info = file.readlines()
    betting_info_list = [line.strip() for line in betting_info]

    return betting_info_list


# Function to extract odds and probabilities for each game
def extract_odds_and_probabilities(info_list):
    odds_dict = {}
    probabilities_dict = {}
    PIX_index = info_list.index("Fyll i med PIX!")
    spel_guide_index = info_list.index("Spelguide")    
    games_info = info_list[PIX_index+2:spel_guide_index]
    for counter, elem in enumerate(games_info):
        
        if elem == '-':
            home_team = games_info[counter - 1]
            away_team = games_info[counter + 1]
        if elem == 'Svenska folket':
            probs = list(map(lambda a: float(a.split('%')[0])/100, games_info[counter+1:counter+4]))
        if elem == 'Odds':
            odds = list(map(lambda a: float(a.replace(',', '.')),games_info[counter+1:counter+4]))
        
            odds_dict[away_team + ' vs ' + home_team] = odds
            probabilities_dict[away_team + ' vs ' + home_team] = probs
            
            
    return odds_dict, probabilities_dict

def get_total_turnover(betting_info):
    for i, elem in enumerate(betting_info):
        if elem.lower() == 'omsättning':
            turnover_index = i+1
    #Get numeric part
    numeric_part = betting_info[turnover_index].split('kr')[0].strip()
    #Remove spaces
    turnover = ""
    for integer in numeric_part:
        if integer != " ":
            turnover+=integer
    return int(turnover)
def get_games(betting_info:list, num_games=8):
    pix_index = betting_info.index('Fyll i med PIX!')
    spelguide_index = betting_info.index('Spelguide')

    games = []

    first_team_index  = pix_index + 2
    second_team_index = first_team_index + 2

    for team_index in range(2, spelguide_index-14, 15):
        first_team_index = pix_index + team_index
        second_team_index = first_team_index + 2

        game = f"{betting_info[first_team_index]}-{betting_info[second_team_index]}"
        games.append(game)
    
    return games 
    
def main():
    tips_type = 'stryktipset'
    scrape_svenska_spel(tips_type=tips_type)
    file_title = create_filename_with_date(tipstype=tips_type)
    betting_info = read_betting_info(file_title=file_title)
    total_turnover = get_total_turnover(betting_info)

    odds, probs = extract_odds_and_probabilities(betting_info)
    games  = get_games(betting_info)
    
    enum_odds = []
    enum_probs = []
    
    for i, key in enumerate(odds.keys()):
        for i in odds[key]:
            enum_odds.append(i)
        
    for i, key in enumerate(probs.keys()):
        for i in probs[key]:
            enum_probs.append(i)
    
    assert (len(enum_odds) == len(enum_probs))    
    
    
    #Where to store data
    data_dir = "data"
    cwd = os.listdir()
    if data_dir not in cwd:
        os.mkdir(data_dir)
    data_dir += '/'

    #n_games = number of odds / 3 - since odds for home win, draw and away win. 
    num_games = int(len(enum_probs) / 3) 
    odds_filename           = data_dir + "odds.bin"
    probs_filename          = data_dir + "probs.bin"
    num_games_filename      = data_dir + "num_games.bin"
    total_turnover_filename = data_dir + "total_turnover.bin"
    teams_in_game_filename  = data_dir + "teams_in_game.bin" 
    #odds
    with open(odds_filename, 'wb') as f:
        array('d', enum_odds).tofile(f)
        
    #probs
    with open(probs_filename, 'wb') as f:
        array('d', enum_probs).tofile(f)

    #turnover
    with open(total_turnover_filename, 'wb') as f:
        f.write(struct.pack("i", total_turnover))
    
    #num games
    with open(num_games_filename, 'wb') as f:
        f.write(struct.pack("i", num_games))
    
    #games
    with open(teams_in_game_filename, "wb") as f:
        f.write(struct.pack("I", len(games)))  
        for s in games:
            encoded_s = s.encode("utf-8") 
            f.write(struct.pack("I", len(encoded_s))) 
            f.write(encoded_s)

if __name__ == "__main__":
    main()