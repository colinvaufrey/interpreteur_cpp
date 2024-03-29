#include "Interpreteur.h"
#include <stdlib.h>
#include <iostream>
using namespace std;

Interpreteur::Interpreteur(ifstream & fichier) :
m_lecteur(fichier), m_table(), m_arbre(nullptr) {
    m_textAda = false;
}

void Interpreteur::traduitEnAda(ofstream & f, string nomProc) const {
    if (this->isTextAda()) {
        f << "with Ada.Text_IO, Ada.Integer_Text_IO;" << endl << "use Ada.Text_IO, Ada.Integer_Text_IO;" << endl << endl;
    }

    f << "procedure " + nomProc + " is" << endl;
    unsigned int indSymbole = 0;
    while (indSymbole < this->getTable().getTaille()) {
        SymboleValue symbole = this->getTable()[indSymbole];
        if (symbole == "<VARIABLE>") // Si le symbole n'est pas une chaine
            f << setw(4) << "" << symbole.getChaine() << " : integer;" << endl;
        indSymbole++;
    }
    f << "begin" << endl;
    this->getArbre()->traduitEnAda(f, 1);
    f << "end " + nomProc + ";";
}

void Interpreteur::analyse() {
  m_arbre = programme(); // on lance l'analyse de la première règle
}

void Interpreteur::tester(const string & symboleAttendu) const {
  // Teste si le symbole courant est égal au symboleAttendu... Si non, lève une exception
  static char messageWhat[256];
  if (m_lecteur.getSymbole() != symboleAttendu) {
    sprintf(messageWhat,
            "Ligne %d, Colonne %d - Erreur de syntaxe - Symbole attendu : %s - Symbole trouvé : %s",
            m_lecteur.getLigne(), m_lecteur.getColonne(),
            symboleAttendu.c_str(), m_lecteur.getSymbole().getChaine().c_str());
    throw SyntaxeException(messageWhat);
  }
}

void Interpreteur::testerEtAvancer(const string & symboleAttendu) {
  // Teste si le symbole courant est égal au symboleAttendu... Si oui, avance, Sinon, lève une exception
  tester(symboleAttendu);
  m_lecteur.avancer();
}

void Interpreteur::erreur(const string & message) const {
  // Lève une exception contenant le message et le symbole courant trouvé
  // Utilisé lorsqu'il y a plusieurs symboles attendus possibles...
  static char messageWhat[256];
  sprintf(messageWhat,
          "Ligne %d, Colonne %d - Erreur de syntaxe - %s - Symbole trouvé : %s",
          m_lecteur.getLigne(), m_lecteur.getColonne(), message.c_str(), m_lecteur.getSymbole().getChaine().c_str());
  throw SyntaxeException(messageWhat);
}

Noeud* Interpreteur::programme() {
  // <programme> ::= procedure principale() <seqInst> finproc FIN_FICHIER
   m_nbErreurs = 0;
    
  testerEtAvancer("procedure");
  testerEtAvancer("principale");
  testerEtAvancer("(");
  testerEtAvancer(")");
  Noeud* sequence = seqInst();
  testerEtAvancer("finproc");
  tester("<FINDEFICHIER>");
  return sequence;
}

Noeud* Interpreteur::seqInst() {
  // <seqInst> ::= <inst> { <inst> }
  NoeudSeqInst* sequence = new NoeudSeqInst();
  do {
    sequence->ajoute(inst());
  } while (m_lecteur.getSymbole() == "<VARIABLE>" || m_lecteur.getSymbole() == "si"
          || m_lecteur.getSymbole() == "tantque" || m_lecteur.getSymbole() == "repeter"
          || m_lecteur.getSymbole() == "pour" || m_lecteur.getSymbole() == "ecrire"
          || m_lecteur.getSymbole() == "lire");
  // Tant que le symbole courant est un début possible d'instruction...
  // Il faut compléter cette condition chaque fois qu'on rajoute une nouvelle instruction
  return sequence;
}

Noeud* Interpreteur::inst() {
  // <inst> ::= <affectation>  ; | <instSi>
    try {
        if (m_lecteur.getSymbole() == "<VARIABLE>") {
            Noeud *affect = affectation();
            testerEtAvancer(";");
            return affect;
        }
        else if (m_lecteur.getSymbole() == "si")
            return instSiRiche();
        // Compléter les alternatives chaque fois qu'on rajoute une nouvelle instruction
        else if (m_lecteur.getSymbole() == "tantque")
            return instTantQue();
        else if (m_lecteur.getSymbole() == "repeter")
            return instRepeter();
        else if (m_lecteur.getSymbole() == "pour")
            return instPour();
        else if (m_lecteur.getSymbole() == "ecrire")
            return instEcrire();
        else if (m_lecteur.getSymbole() == "lire")
            return instLire();
        else {
            erreur("Instruction incorrecte");
            return nullptr;
        }
    } catch (SyntaxeException e) {
        m_nbErreurs++;
        addException(e.what());
        
        while (m_lecteur.getSymbole() != "<VARIABLE>" && m_lecteur.getSymbole() != "si"
            && m_lecteur.getSymbole() != "tantque" && m_lecteur.getSymbole() != "repeter"
            && m_lecteur.getSymbole() != "pour" && m_lecteur.getSymbole() != "ecrire"
            && m_lecteur.getSymbole() != "lire" && m_lecteur.getSymbole() != "<FINDEFICHIER>"
            && m_lecteur.getSymbole() != "finproc") {
            m_lecteur.avancer();
        }
        
        return nullptr;
    }
}

Noeud* Interpreteur::affectation() {
  // <affectation> ::= <variable> = <expression> 
  tester("<VARIABLE>");
  Noeud* var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table eton la mémorise
  m_lecteur.avancer();
  Noeud* exp;
  if (m_lecteur.getSymbole() == "=") {
    testerEtAvancer("=");
    exp = expression();             // On mémorise l'expression trouvée
  } else if (m_lecteur.getSymbole() == "++") {
    testerEtAvancer("++");
    Symbole op("+");
    Symbole sUn("1");
    SymboleValue svUn(sUn);
    svUn.setValeur(1);
    Noeud* un = m_table.chercheAjoute(svUn);
    exp = new NoeudOperateurBinaire(op, var, un);
  } else if (m_lecteur.getSymbole() == "--") {
    testerEtAvancer("--");
    Symbole op("-");
    Symbole sUn("1");
    SymboleValue svUn(sUn);
    svUn.setValeur(1);
    Noeud* un = m_table.chercheAjoute(svUn);
    exp = new NoeudOperateurBinaire(op, var, un);
  } else {
    testerEtAvancer("=");
  }
  return new NoeudAffectation(var, exp); // On renvoie un noeud affectation
}

Noeud* Interpreteur::expression() {
//  <expression> ::= <expEt> {ou <expEt> }
    Noeud* exp = expEt();
    while (m_lecteur.getSymbole() == "ou") {
        Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
        m_lecteur.avancer();
        Noeud* expDroite = expEt(); // On mémorise l'expression de droite
        exp = new NoeudOperateurBinaire(operateur, exp, expDroite); // Et on construit un noeud opérateur binaire
    }
    return exp;
}
Noeud* Interpreteur::expEt() {
//  <expEt> ::= <expComp> {et <expComp> }
    Noeud* exp = expComp();
    while (m_lecteur.getSymbole() == "et") {
        Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
        m_lecteur.avancer();
        Noeud* expDroite = expComp(); // On mémorise l'expression de droite
        exp = new NoeudOperateurBinaire(operateur, exp, expDroite); // Et on construit un noeud opérateur binaire
    }
    return exp;
}

Noeud* Interpreteur::expComp() {
//  <expComp> ::= <expAdd> {==|!=|<|<=|>|>= <expAdd> }
    Noeud* exp = expAdd();
    while ( m_lecteur.getSymbole() == "==" || m_lecteur.getSymbole() == "!=" ||
            m_lecteur.getSymbole() == "<"  || m_lecteur.getSymbole() == "<=" ||
            m_lecteur.getSymbole() == ">"  || m_lecteur.getSymbole() == ">=" ) {
        Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
        m_lecteur.avancer();
        Noeud* expDroite = expAdd(); // On mémorise l'expression de droite
        exp = new NoeudOperateurBinaire(operateur, exp, expDroite); // Et on construit un noeud opérateur binaire
    }
    return exp;
}

Noeud* Interpreteur::expAdd() {
//  <expAdd> ::= <expMult> {+|-<expMult> }
    Noeud* exp = expMult();
    while (m_lecteur.getSymbole() == "+"  || m_lecteur.getSymbole() == "-") {
        Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
        m_lecteur.avancer();
        Noeud* expDroite = expMult(); // On mémorise l'expression de droite
        exp = new NoeudOperateurBinaire(operateur, exp, expDroite); // Et on construit un noeud opérateur binaire
    }
    return exp;    
}

Noeud* Interpreteur::expMult() {
//  <expMult>::= <facteur> {*|/<facteur> }
    Noeud* fact = facteur();
    while (m_lecteur.getSymbole() == "*"  || m_lecteur.getSymbole() == "/") {
        Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
        m_lecteur.avancer();
        Noeud* factDroit = facteur(); // On mémorise l'expression de droite
        fact = new NoeudOperateurBinaire(operateur, fact, factDroit); // Et on construit un noeud opérateur binaire
    }
    return fact;
}


Noeud* Interpreteur::facteur() {
  // <facteur> ::= <entier> | <variable> | - <facteur> | non <facteur> | ( <expression> )
  Noeud* fact = nullptr;
  if (m_lecteur.getSymbole() == "<VARIABLE>" || m_lecteur.getSymbole() == "<ENTIER>") {
    fact = m_table.chercheAjoute(m_lecteur.getSymbole()); // on ajoute la variable ou l'entier à la table
    m_lecteur.avancer();
  } else if (m_lecteur.getSymbole() == "-") { // - <facteur>
    m_lecteur.avancer();
    // on représente le moins unaire (- facteur) par une soustraction binaire (0 - facteur)
    fact = new NoeudOperateurBinaire(Symbole("-"), m_table.chercheAjoute(Symbole("0")), facteur());
  } else if (m_lecteur.getSymbole() == "non") { // non <facteur>
    m_lecteur.avancer();
    // on représente le moins unaire (- facteur) par une soustractin binaire (0 - facteur)
    fact = new NoeudOperateurBinaire(Symbole("non"), facteur(), nullptr);
  } else if (m_lecteur.getSymbole() == "(") { // expression parenthésée
    m_lecteur.avancer();
    fact = expression();
    testerEtAvancer(")");
  } else
    erreur("Facteur incorrect");
  return fact;
}

//Noeud* Interpreteur::instSi() {
//  // <instSi> ::= si ( <expression> ) <seqInst> finsi
//  testerEtAvancer("si");
//  testerEtAvancer("(");
//  Noeud* condition = expression(); // On mémorise la condition
//  testerEtAvancer(")");
//  Noeud* sequence = seqInst();     // On mémorise la séquence d'instruction
//  if (m_lecteur.getSymbole() == "sinonsi" || m_lecteur.getSymbole() == "sinon") {
//      Noeud* = instSiRiche();
//      return nullptr; // Return NoeudInstSiRiche
//  } else {
//    testerEtAvancer("finsi");
//    return new NoeudInstSi(condition, sequence); // Et on renvoie un noeud Instruction Si
//  }
//}

Noeud* Interpreteur::instSiRiche() {
// <instSiRiche> ::= si (<expression>) <seqInst> { sinonsi (<expression>) <seqInst> } [sinon <seqInst>] finsi
    NoeudInstSiRiche* noeudSiRiche = new NoeudInstSiRiche();
    
    testerEtAvancer("si");
    testerEtAvancer("(");
    Noeud* condition = expression();
    noeudSiRiche->ajouterCondition(condition);
    testerEtAvancer(")");
    Noeud* sequence = seqInst();
    noeudSiRiche->ajouterSequence(sequence);
    while (m_lecteur.getSymbole() == "sinonsi") {
        testerEtAvancer("sinonsi");
        testerEtAvancer("(");
        Noeud* condition = expression();
        noeudSiRiche->ajouterCondition(condition);
        testerEtAvancer(")");
        Noeud* sequence = seqInst();
        noeudSiRiche->ajouterSequence(sequence);
    }
    if(m_lecteur.getSymbole() == "sinon") {
        testerEtAvancer("sinon");
        Noeud* sequence = seqInst();
        noeudSiRiche->ajouterSequence(sequence);
    }
    testerEtAvancer("finsi");
    return noeudSiRiche;
}

Noeud* Interpreteur::instTantQue() {
// <instTantQue> ::= tantque( <expression> ) <seqInst> fintantque
    testerEtAvancer("tantque");
    testerEtAvancer("(");
    Noeud* condition = expression();
    testerEtAvancer(")");
    Noeud* sequence = seqInst();
    testerEtAvancer("fintantque");
    
    return new NoeudInstTantQue(condition, sequence);
}  

Noeud* Interpreteur::instRepeter() {
// <instRepeter> ::= repeter <seqInst> jusqua ( <expression> )
    testerEtAvancer("repeter");
    Noeud* sequence = seqInst();
    testerEtAvancer("jusqua");
    testerEtAvancer("(");
    Noeud* condition = expression();
    testerEtAvancer(")");
    return new NoeudInstRepeter(condition, sequence);
}

Noeud* Interpreteur::instPour() {
// <instPour>    ::= pour( [ <affectation> ] ; <expression> ;[ <affectation> ]) <seqInst> finpour
    
    Noeud* affectInit = nullptr;
    Noeud* affectBoucle = nullptr;
    
    testerEtAvancer("pour");
    testerEtAvancer("(");
    if (m_lecteur.getSymbole() == "<VARIABLE>") {
       affectInit = affectation();
    }
    testerEtAvancer(";");
    Noeud* condition = expression();
    testerEtAvancer(";");
    if (m_lecteur.getSymbole() == "<VARIABLE>") {
       affectBoucle = affectation();
    }
    testerEtAvancer(")");
    Noeud* sequence = seqInst();
    testerEtAvancer("finpour");
    
    NoeudInstPour* noeudPour = new NoeudInstPour(condition, sequence);
    
    noeudPour->setAffectationInit(affectInit);
    noeudPour->setAffectationBoucle(affectBoucle);
    
    return noeudPour;
}

Noeud* Interpreteur::instEcrire() {
// <instEcrire>  ::= ecrire( <expression> | <chaine> {, <expression> | <chaine> })
    NoeudInstEcrire* noeudEcrire = new NoeudInstEcrire();
    Noeud* valeur;
    
    this->m_textAda = true;
    
    testerEtAvancer("ecrire");
    testerEtAvancer("(");
    if (m_lecteur.getSymbole() == "<CHAINE>") {
        valeur = m_table.chercheAjoute(m_lecteur.getSymbole()); // La chaine est ajoutée à la table et on la mémorise
        m_lecteur.avancer();
    } else {
        valeur = expression();
    }
    noeudEcrire->ajouterExpression(valeur);
    while (m_lecteur.getSymbole() == ",") {
        m_lecteur.avancer();
        if (m_lecteur.getSymbole() == "<CHAINE>") {
            valeur = m_table.chercheAjoute(m_lecteur.getSymbole()); // La chaine est ajoutée à la table et on la mémorise
            m_lecteur.avancer();
        } else {
            valeur = expression();
        }
        noeudEcrire->ajouterExpression(valeur);
    }
    testerEtAvancer(")");
    testerEtAvancer(";");
    
    return noeudEcrire;
}   

Noeud* Interpreteur::instLire() {
// <instLire>    ::= lire( <variable> {, <variable> })
    NoeudInstLire* noeudLire = new NoeudInstLire();
    Noeud* variable;
    
    this->m_textAda = true;
    
    testerEtAvancer("lire");
    testerEtAvancer("(");
    variable = m_table.chercheAjoute(m_lecteur.getSymbole()); // on ajoute la variable à la table
    noeudLire->ajouterVariable(variable);
    m_lecteur.avancer();
    while (m_lecteur.getSymbole() == ",") {
        m_lecteur.avancer();
        variable = m_table.chercheAjoute(m_lecteur.getSymbole()); // on ajoute la variable à la table
        noeudLire->ajouterVariable(variable);
        m_lecteur.avancer();
    }
    testerEtAvancer(")");
    testerEtAvancer(";");
    
    return noeudLire;
}
