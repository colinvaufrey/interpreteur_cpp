#include <iostream>
#include <stdlib.h>
using namespace std;
#include "Interpreteur.h"
#include "Exceptions.h"

int main(int argc, char* argv[]) {
    string nomFich;
    if (argc != 2) {
        cout << "Usage : " << argv[0] << " nom_fichier_source" << endl << endl;
        cout << "Entrez le nom du fichier que voulez-vous interpréter : ";
        getline(cin, nomFich);
    } else
        nomFich = argv[1];
    ifstream fichier(nomFich.c_str());
    
    if (fichier.fail()) throw FichierException();
    Interpreteur interpreteur(fichier);
    interpreteur.analyse();
    // Si pas d'exception levée, l'analyse syntaxique a réussi
    
    
    if (interpreteur.getNbErreurs() != 0) {
        cout << endl << "================ Syntaxe Erronnée" << endl;
        for (int i = 0; i < interpreteur.getListeExceptions().size(); i++) {
            cout << interpreteur.getListeExceptions()[i] << endl;
        }
    } else {
        cout << endl << "================ Syntaxe Correcte" << endl;
    }
    
    // On affiche le contenu de la table des symboles avant d'exécuter le programme
    cout << endl << "================ Table des symboles avant exécution : " << interpreteur.getTable();

    if (interpreteur.getArbre() != nullptr && interpreteur.getNbErreurs() == 0) {
        cout << endl << "================ Execution de l'arbre" << endl;
        interpreteur.getArbre()->executer();
        // On exécute le programme si l'arbre n'est pas vide
        // Et on vérifie qu'il a fonctionné en regardant comment il a modifié la table des symboles
        cout << endl << "================ Table des symboles apres exécution : " << interpreteur.getTable();
        
        // Réalisation du fichier .adb correspondant à la transcription du code en Ada
        
        // on cherche le dernier point du nom de fichier pour remplacer l'extension (on ajoute juste l'extension si le fichier d'origine n'en a pas)
        
        unsigned int indNom = nomFich.size();
        bool pointFound = false;
        do {
            indNom--;
            if (nomFich.at(indNom) == '.')
                pointFound = true;
         } while (indNom > 0 && !pointFound);
         
         if (!pointFound)
             indNom = nomFich.size();
        
        // on crée le nouveau fichier (ou on l'écrase)
        
        string nomProcedure = nomFich.substr(0, indNom); 
        string nomFicAda = (nomProcedure + ".adb");
        ofstream fichierAda(nomFicAda.c_str());
        
        if (interpreteur.isTextAda()) {
            fichierAda << "with Ada.Text_IO, Ada.Integer_Text_IO;" << endl << "use Ada.Text_IO, Ada.Integer_Text_IO;" << endl << endl;
        }
        
        fichierAda << "procedure " + nomProcedure + " is" << endl;
        unsigned int indSymbole = 0;
        while (indSymbole < interpreteur.getTable().getTaille()) {
            SymboleValue symbole = interpreteur.getTable()[indSymbole];
            if (symbole == "<VARIABLE>") // Si le symbole n'est pas une chaine
                fichierAda << setw(4) << "" << symbole.getChaine() << " : integer;" << endl;
            indSymbole++;
        }
        fichierAda << "begin" << endl;
        interpreteur.getArbre()->traduitEnAda(fichierAda, 1);
        fichierAda << "end " + nomProcedure + ";";
        
        fichierAda.close();
        
        system(("gnatmake -gnatv -gnato " + nomFicAda).c_str());
    }
    
    return 0;
}
