"""Module containing the only function of the whole package: 'run_umodl'."""

import subprocess
import importlib.resources
import platform

umodl = importlib.resources.files("umodl") / ("umodl.exe" if platform.system() == "Windows" else "umodl")

def run_umodl(txtfile, kdicfile, kdictname, treatmentcolname, targetcolname, maxpartnumber = None):
    """Run the umodl executable.

    Parameters
    ----------
    txtfile: path-like
        Path to the Khiops .txt file containing the data.
    kdicfile: path-like
        Path to the Khiops .kdic file containing the Khiops Dictionary describing the data columns.
    kdictname: str
        The name of the Khiops Dictionary as found in kdicfile.
    treatmentcolname: str
        The name of the treatment column as found in txtfile and kdicfile.
    targetcolname: str
        The name of the target column as found in txtfile and kdicfile.
    maxpartnumber: int, default=None
        The maximal number of intervals or groups. None means default to the 'umodl' program default.

    Returns
    -------
    subprocess.CompletedProcess:
        The completed subprocess that has been run.

    Raises
    ------
    subprocess.CalledProcessError:
        If an error happened during the execution of the umodl executable.
    """
    return subprocess.run(filter(None, [umodl, txtfile, kdicfile, kdictname, treatmentcolname, targetcolname, str(maxpartnumber)]), check=True)
