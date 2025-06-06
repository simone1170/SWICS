component_actions = {               # Actions to be performed by this script
    'simulate' : True,
    'transcribe' : False,
    'train' : False,
    'run' : False,
    'evaluate' : False,             # Plot performance of IIDS
    'plot' : True,                   # Plot metrics from physical process
}

ids_config = """
{
    "InterArrivalTimeMean": {
        "N": 6,
        "W": 10,
        "_type": "InterArrivalTimeMean",
        "model-file": "model_file_location"
    }
}
"""

# Default parameter values. Unless a scenario specifies own parameters these are used

# Not inlcuded: createlogfiles, logPcap, outputPath, attackNumber, attackInterval, loggingLevel, loggingGroup, 
# includeLoggingComponents, excludeLoggingComponents, runAttacks (replaced by attackName and attackSchedule)

# Additionally: namingConvention, attackSchedule (list of start and endtimes of attacks) and
# attackName (name of the attack expected by sim, default will be the dict key of the attack dict)

wired_config = {
    'use5g' : 'false',
    'duration' : 1200,
    'attackSchedule' : [(100, 200), (300, 400), (500, 600), (700, 800), (900,1000)],
    'attackName' : '', # Default: Name of the dict key
    'jammerInside' : 'false',
    'jammerDirected' : 'false',
    'jammerPower' : 0,
    'bjp' : 0,
    'dutyCycle' : 1
}

wireless_config = wired_config.copy()
wireless_config.update({'use5g' : 'true'})

directed_config = wireless_config.copy()
directed_config.update({'jammerDirected' : 'true'})

# Attacks and their respective config
# Missing entries will be filled in by default config

general_attacks = {
    'Baseline' : {},
    'DoS' : {},
    'Suppression' : {},
    'Injection' : {},
    'MitM48msg' : {}
}

wireless_attacks = {
    'JammingOutside-MediumHighDuty' : {
        'use5g' : 'true',
        'attackName' : 'Jammer',
        'jammerInside' : 'false',
        'dutyCycle' : 0.75
    },
    'JammingOutside-MediumDuty' : {
        'use5g' : 'true',
        'attackName' : 'Jammer',
        'jammerInside' : 'false',
        'dutyCycle' : 0.5
    },
    'JammingOutside-LowDuty' : {
        'use5g' : 'true',
        'attackName' : 'Jammer',
        'jammerInside' : 'false',
        'dutyCycle' : 0.25
    }
}

# Format: <scenario_name> : ({<parameter> : <value_list>}, attack_dict, default_config)
# => For scenario_name vary the parameter for all values from value_list
#    For each parameter value execute all attacks from attack_dict with their respective attack config

# 5g-dj uses directed jammer; 5g-uj regular, undirected jammer
scenarios = {
    '5g-dj' : ({'jammerPower' : [20, 22, 26, 28, 30, 32, 34, 38, 40]}, wireless_attacks, directed_config),
    '5g-uj' : ({'jammerPower' : [33, 48, 52, 56, 64, 68, 72, 80, 82, 84, 88, 96, 100, 104, 108]}, wireless_attacks, wireless_config),
}
