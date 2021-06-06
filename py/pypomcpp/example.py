import pommerman
import pommerman.agents as agents
from pommerman.agents.simple_agent import SimpleAgent
from pypomcpp.cppagent import CppAgent
from util import ffa_evaluate
from shutil import copyfile

lib_path = "./Release/libpomcpp.so"

def create_lib_copy():
    if hasattr(create_lib_copy, "calls"):
        create_lib_copy.calls += 1
    else:
        create_lib_copy.calls = 0

    local_lib_path = f"./local_lib_copy_{create_lib_copy.calls}.so"
    copyfile(lib_path, local_lib_path)
    return local_lib_path

agent_list = [
    #agents.SimpleAgent(),
    #agents.SimpleAgent(),
    #agents.SimpleAgent(),
    CppAgent(create_lib_copy(), "SimpleAgent", seed=14, print_json=False),
    CppAgent(create_lib_copy(), "SimpleAgent", seed=15),
    CppAgent(create_lib_copy(), "SimpleAgent", seed=16),
    CppAgent(create_lib_copy(), "SimpleAgent", seed=17),
]

# Make the "Free-For-All" environment using the agent list
env = pommerman.make('PommeFFACompetition-v0', agent_list)

use_env_state = False

if use_env_state:
    for a in agent_list:
        if isinstance(a, CppAgent):
            a.use_env_state(env)

ffa_evaluate(env, 10, True, False)
